#pragma once
#include "base.h"

namespace ez {
    class Cart {
    public:
        Cart(const fs::path& path);

        const uint8_t* data(uint16_t byteOffset) const;

    private:
        uint16_t m_sizeBytes = 0;
        std::vector<uint8_t> m_data;
    };
}
