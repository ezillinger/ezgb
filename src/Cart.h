#pragma once
#include "Base.h"

namespace ez {
    class Cart {
    public:
        Cart(const fs::path& path);
        Cart(const uint8_t* data, size_t len);

        const uint8_t* data(uint16_t byteOffset) const;

    private:
        uint16_t m_sizeBytes = 0;
        std::vector<uint8_t> m_data;
    };
}
