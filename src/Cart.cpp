#include "Cart.h"

namespace ez {

Cart Cart::loadFromDisk(const fs::path& path) {
    log_info("Opening cart: {}", path.string());
    EZ_ENSURE(fs::exists(path));

    const auto sizeBytes = fs::file_size(path);
    EZ_ENSURE(sizeBytes > 0);
    auto data = std::vector<uint8_t>(size_t(sizeBytes));
    auto fp = fopen(path.generic_string().c_str(), "rb");
    fread(data.data(), 1, sizeBytes, fp);
    fclose(fp);

    return Cart(data.data(), data.size());
}

Cart::Cart(const uint8_t* data, size_t len) : m_sizeBytes(len), m_data(data, data + len) {
    m_cartType = CartType(m_data[0x0147]);
    log_info("CartType: {}", +m_cartType);
    EZ_ASSERT(m_cartType == CartType::ROM_ONLY || m_cartType == CartType::MBC1);
}

void Cart::writeAddr(uint16_t addr, uint8_t val) {
    EZ_ASSERT(isValidAddr(addr));
    if (m_cartType == CartType::ROM_ONLY) {
        log_warn("Can't write to ROM only cart!");
    } else if (m_cartType == CartType::MBC1) {
        if (addr <= 0x1FFF) {
            m_mbc1State.m_ramEnable = val;
        } else if (addr >= 0x2000 && addr < 0x3FFF) {
            m_mbc1State.m_romBankSelect = val & 0b00011111; // only 5 bits used
        } else if (addr >= 0x4000 && addr < 0x5FFF) {
            m_mbc1State.m_ramBankSelect = val;
        } else if (addr >= 0x6000 && addr < 0x7FFF) {
            m_mbc1State.m_romRamModeSelect = val;
        } else if (RAM_RANGE.containsExclusive(addr)) {
            *getRAMPtr(addr) = val;
        } else {
            EZ_FAIL("Wat?");
        }
    }
}
void Cart::writeAddr16(uint16_t addr, uint16_t /*val*/) {
    EZ_ASSERT(isValidAddr(addr));
    EZ_FAIL("Please tell me carts don't support 16 bit writes");
}

uint8_t Cart::readAddr(uint16_t addr) const {
    EZ_ASSERT(isValidAddr(addr));
    if (m_cartType == CartType::ROM_ONLY) {
        return *getROMPtr(addr);
    } else if (m_cartType == CartType::MBC1) {
        if (RAM_RANGE.containsExclusive(addr)) { // RAM
            if (m_mbc1State.isRamEnabled()) {
                return *getRAMPtr(addr);
            } else {
                EZ_ASSERT(ROM_RANGE.containsExclusive(addr));
                log_warn("Reading from disabled RAM!");
                return 0xFF;
            }
        } else { // ROM
            return *getROMPtr(addr);
        }
    } else {
        EZ_FAIL("Not implemented");
    }
}

uint16_t Cart::readAddr16(uint16_t addr) const {
    EZ_ASSERT(isValidAddr(addr));
    return *reinterpret_cast<const uint16_t*>(getROMPtr(addr));
};

const uint8_t* Cart::getROMPtr(uint16_t addr) const {
    EZ_ASSERT(isValidAddr(addr));
    if (m_cartType == CartType::ROM_ONLY) {
        return m_data.data() + addr;
    } else if (m_cartType == CartType::MBC1) {
        if (RAM_RANGE.containsExclusive(addr)) {
            EZ_FAIL("This is a ROM address!");
        } else {
            EZ_ASSERT(ROM_RANGE.containsExclusive(addr));
            if (addr <= 0x3FFF) { // bank 0
                return m_data.data() + addr;
            } else {
                const auto bankSelect = std::max(m_mbc1State.m_romBankSelect, uint8_t(1));
                const int bankedAddr = (addr - 0x4000) + (bankSelect * 0x4000);
                // todo, support 2 bits from rom/ram mode
                return m_data.data() + bankedAddr;
            }
        }
    } else {
        EZ_FAIL("NOT IMPLEMENTED");
    }
}

} // namespace ez