#include "Cart.h"

namespace ez {

Cart Cart::load_from_disk(const fs::path& path) {
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
    ez_assert(m_cartType == CartType::ROM_ONLY || is_mbc_type(m_cartType));
}

void Cart::write_addr(uint16_t addr, uint8_t val) {
    ez_assert(is_valid_addr(addr));
    if (m_cartType == CartType::ROM_ONLY) {
        log_warn("Can't write to ROM only cart!");
    } else if (is_mbc_type(m_cartType)) {
        if (addr <= 0x1FFF) {
            m_mbc1State.m_ramEnable = val;
        } else if (addr >= 0x2000 && addr <= 0x3FFF) {
            m_mbc1State.m_romBankSelect = val & 0b00011111; // only 5 bits used
        } else if (addr >= 0x4000 && addr < 0x5FFF) {
            m_mbc1State.m_ramBankSelect = val;
        } else if (addr >= 0x6000 && addr < 0x7FFF) {
            m_mbc1State.m_romRamModeSelect = val;
        } else if (RAM_RANGE.containsExclusive(addr)) {
            if (m_cartType == CartType::ROM_ONLY) {
                log_error("Write to ROM only RAM addr");
                return;
            }
            *get_ram_ptr(addr) = val;
        } else {
            fail("Wat?");
        }
    }
}

uint8_t Cart::read_addr(uint16_t addr) const {
    ez_assert(is_valid_addr(addr));
    if (m_cartType == CartType::ROM_ONLY) {
        return *get_rom_ptr(addr);
    } else if (is_mbc_type(m_cartType)) {
        if (RAM_RANGE.containsExclusive(addr)) { // RAM
            if (m_mbc1State.is_ram_enabled()) {
                return *get_ram_ptr(addr);
            } else {
                log_warn("Reading from disabled RAM!");
                return 0xFF;
            }
        } else { // ROM
            return *get_rom_ptr(addr);
        }
    } else {
        fail("Not implemented");
    }
}

const uint8_t* Cart::get_rom_ptr(uint16_t addr) const {
    ez_assert(is_valid_addr(addr));
    if (m_cartType == CartType::ROM_ONLY) {
        return m_data.data() + addr;
    } else if (is_mbc_type(m_cartType)) {
        if (RAM_RANGE.containsExclusive(addr)) {
            fail("This is a ROM address!");
        } else {
            ez_assert(ROM_RANGE.containsExclusive(addr));
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
        fail("NOT IMPLEMENTED");
    }
}

bool Cart::is_mbc_type(CartType type) {
    return type == CartType::MBC1 || type == CartType::MBC1_RAM ||
           type == CartType::MBC1_RAM_BATTERY;
}

} // namespace ez