#include "Cart.h"

namespace ez {

    Cart::Cart(const fs::path& path) {
        log_info("Opening cart: {}", path.string());
        EZ_ENSURE(fs::exists(path));

        m_sizeBytes = static_cast<uint16_t>(fs::file_size(path));
        // we'll pad out our cart with 3 zero bytes so we can safelty do a 4 byte read on the last real byte
        m_data = std::vector<uint8_t>(size_t(m_sizeBytes) + 3, 0u);
        auto fp = fopen(path.generic_string().c_str(), "rb");
        fread(m_data.data(), 1, m_sizeBytes, fp);
        fclose(fp);
    }


    Cart::Cart(const uint8_t* data, size_t len)
    : m_data(data, data + len)
    {
        // no padding on this one
    }

    const uint8_t* Cart::data(uint16_t byteOffset) const {
        EZ_ENSURE(byteOffset < m_sizeBytes);
        return m_data.data() + byteOffset;
    }
}