#pragma once
#include "Base.h"

namespace ez {
    inline constexpr bool getFlagHalfCarryAdd(uint8_t a, uint8_t b) { return (((a & 0x0F) + (b & 0x0F)) & 0x10) == 0x10; }
    inline constexpr bool getFlagCarryAdd(uint8_t a, uint8_t b) { return int(a) + b > 0xFF; }
}