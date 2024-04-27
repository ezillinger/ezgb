#pragma once
#include "Base.h"

namespace ez {

inline constexpr bool getFlagHC_ADD(uint8_t a, uint8_t b) {
    return (((a & 0x0F) + (b & 0x0F)) & 0x10) == 0x10;
}
inline constexpr bool getFlagC_ADD(uint8_t a, uint8_t b) { return int(a) + b > 0xFF; }

inline constexpr bool getFlagHC_SUB(uint8_t a, uint8_t b) {
    return ((a & 0xF) - (b & 0xF)) & 0x10;
}
inline constexpr bool getFlagC_SUB(uint8_t a, uint8_t b) { 
    return a < b; 
}

inline constexpr bool getFlagHC_ADC(uint8_t a, uint8_t b, uint8_t carryBit) {
    EZ_ASSERT(carryBit <= 0b1);
    return ((a & b) | ((a ^ b) & ~(a + b + carryBit))) & 0b1000;
}
inline constexpr bool getFlagC_ADC(uint8_t a, uint8_t b, uint8_t carryBit) {
    EZ_ASSERT(carryBit <= 0b1);
    return (int(a) + b + carryBit) > 0xFF;
}

inline constexpr bool getFlagHC_SBC(uint8_t a, uint8_t b, uint8_t carryBit) {
    EZ_ASSERT(carryBit <= 0b1);
    return ((a & 0xF) - (b & 0xF) - (carryBit)) & 0x10;
}
inline constexpr bool getFlagC_SBC(uint8_t a, uint8_t b, uint8_t carryBit) {
    EZ_ASSERT(carryBit <= 0b1);
    return a < (b + carryBit);
}

inline constexpr bool is_tima_increment(uint16_t sysclkBefore, uint16_t sysclkAfter, uint8_t tacBefore, uint8_t tacAfter){
    // if enable bit isn't set no increment
    if(!(tacAfter & 0b100)){
        return false;
    }
    auto getBitIdx = [](uint8_t tac) {
        const auto timaControlBits = 0b11 & tac;
        return timaControlBits == 0b00   ? 9
                                    : timaControlBits == 0b11 ? 7
                                    : timaControlBits == 0b10 ? 5
                                                              : 3;
    };

    const bool bitSetBefore = (sysclkBefore & (0b1 << getBitIdx(tacBefore)));
    const bool bitSetAfter = (sysclkAfter & (0b1 << getBitIdx(tacAfter)));

    return bitSetBefore && !bitSetAfter;
}

} // namespace ez