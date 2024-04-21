#include "Cart.h"

namespace ez {

Cart::Cart(const fs::path& path) {
    log_info("Opening cart: {}", path.string());
    EZ_ENSURE(fs::exists(path));

    m_sizeBytes = fs::file_size(path);
    EZ_ENSURE(m_sizeBytes > 0);
    // we'll pad out our cart with 3 zero bytes so we can safelty do a 4 byte read on the last
    // real byte
    m_data = std::vector<uint8_t>(size_t(m_sizeBytes) + 3, 0u);
    auto fp = fopen(path.generic_string().c_str(), "rb");
    fread(m_data.data(), 1, m_sizeBytes, fp);
    fclose(fp);

    m_cartType = std::bit_cast<CartType>(m_data[0x0147]);
    log_info("CartType: {}", +m_cartType);
}

Cart::Cart(const uint8_t* data, size_t len) : m_data(data, data + len) {
    // no padding on this one
}

const uint8_t* Cart::data(uint16_t byteOffset) const {
    EZ_ENSURE(byteOffset < m_sizeBytes);
    return m_data.data() + byteOffset;
}

bool Cart::isReadOnlyAddr(uint16_t addr) { 
    switch (m_cartType)
    {
    case CartType::ROM_ONLY:
        return addr < 0x4000; 
    default:
        return addr < 0x4000; 
    }
}

void Cart::writeAddr(uint16_t addr, uint8_t val) {
    if (isReadOnlyAddr(addr)) {
        log_error("Write to R/O ROM addr: {}", addr);
        return;
    }
    m_data[addr] = val;
}
void Cart::writeAddr16(uint16_t addr, uint16_t val) {
    if (isReadOnlyAddr(addr)) {
        log_error("Write to R/O ROM addr: {}", addr);
        return;
    }
    *reinterpret_cast<uint16_t*>(m_data.data() + addr) = val;
}
} // namespace ez