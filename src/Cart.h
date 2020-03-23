#pragma once
#include "base.h"

namespace ez {
    class Cart {
    public:
        Cart(const fs::path& path);

        uint32_t read32(uint16_t byteOffset);

    private:
        uint16_t m_sizeBytes = 0;
        std::vector<uint8_t> m_data;
    };
}
