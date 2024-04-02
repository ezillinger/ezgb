#pragma once
#include "Base.h"

namespace ez {
class Tester;

class IO {

  public:
    friend class Tester;
    static constexpr uint16_t IO_BASE_ADDR = 0xFF00;
    static constexpr size_t IO_BYTES = 128;

    bool is_bootrom_mapped() const { 
        return m_reg.m_bootromDisabled; 
    };

    uint8_t* getMemPtrRW(uint16_t address);
    const uint8_t* getMemPtr(uint16_t address) const;

    uint8_t& getMem8RW(uint16_t address) { return *getMemPtrRW(address); }
    uint8_t getMem8(uint16_t address) const;

private:

    struct alignas(uint8_t) {
        uint8_t m_joypad {};
        uint8_t m_serial[2] {};
        uint8_t detail_padding0{};
        uint8_t m_timerDivider[4]{};
        uint8_t detail_padding1[8]{};
        uint8_t m_audio[23]{};
        uint8_t detail_padding2[9]{};
        uint8_t m_wavePattern[16]{};
        uint8_t m_lcd[12]{};
        uint8_t detail_padding3[3]{};
        uint8_t m_vramBankSelect{}; // CGB only
        bool m_bootromDisabled = false;
        uint8_t m_vramDMA[5]{}; // CGB only
        uint8_t detail_padding4[18]{};
        uint8_t m_bgObjPalettes[4]{}; // CGB only
        uint8_t detail_padding5[4]{};
        uint8_t m_wramBankSelect{};
        uint8_t detail_padding[15]{};
    } m_reg;
    static_assert(sizeof(m_reg) == IO_BYTES);
};
} // namespace ez