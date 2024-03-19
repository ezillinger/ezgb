#include "Cart.h"

namespace ez {

    Cart::Cart(const fs::path& path) {
        log_info("Opening cart: {}", path.string());
        EZ_ENSURE(fs::exists(path));

        m_sizeBytes = static_cast<uint16_t>(fs::file_size(path));
        // we'll pad out our cart with 3 zero bytes so we can safelty do a 32 bit read on the last real byte
        m_data = std::vector<uint8_t>(size_t(m_sizeBytes) + 3, 0u);
        auto fp = fopen(path.generic_string().c_str(), "rb");
        fread(m_data.data(), 1, m_sizeBytes, fp);
    }

    uint32_t Cart::read32(uint16_t byteOffset) {
        EZ_ENSURE(byteOffset < m_sizeBytes);
        return *reinterpret_cast<uint32_t*>(m_data.data() + byteOffset);
    }
}