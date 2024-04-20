#pragma once
#include "Base.h"

namespace ez {
    class Cart {
    public:
        Cart(const fs::path& path);
        Cart(const uint8_t* data, size_t len);

        const uint8_t* data(uint16_t byteOffset) const;
        uint8_t readAddr(uint16_t addr) const { return m_data[addr]; }
        uint16_t readAddr16(uint16_t addr) const {
            return *reinterpret_cast<const uint16_t*>(m_data.data() + addr);
        };

      private:
        size_t m_sizeBytes = 0ull;
        std::vector<uint8_t> m_data;
    };
}
