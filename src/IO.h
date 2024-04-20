#pragma once
#include "Base.h"
#include "PPU.h"

namespace ez {
class Tester;

class IO {

  public:
    friend class Tester;

    static constexpr uint16_t IO_BASE_ADDR = 0xFF00;
    static constexpr size_t IO_BYTES = 128;

    bool isBootromMapped() const { return !m_reg.m_bootromDisabled; };
    void setBootromMapped(bool val) { m_reg.m_bootromDisabled = val; }


    void writeAddr(uint16_t addr, uint8_t val);
    uint8_t readAddr(uint16_t address) const;

    LCDRegisters& getLCDRegisters() { return m_reg.m_lcd; }

    const std::string& getSerialOutput() { return m_serialOutput; }

  private:

    // for testing only
    uint8_t* getMemPtrRW(uint16_t address);

    struct alignas(uint8_t) {
        uint8_t m_joypad{};
        uint8_t m_serial[2]{};
        uint8_t detail_padding0{};
        uint8_t m_timerDivider[4]{};
        uint8_t detail_padding1[8]{};
        uint8_t m_audio[23]{};
        uint8_t detail_padding2[9]{};
        uint8_t m_wavePattern[16]{};
        LCDRegisters m_lcd{};
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

    std::string m_serialOutput;
};
} // namespace ez