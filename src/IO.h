#pragma once
#include "Base.h"

namespace ez {
class Tester;

enum class PPUMode { OAM_SCAN = 2, DRAWING = 3, HBLANK = 0, VBLANK = 1 };

struct alignas(uint8_t) LCDRegisters {
    struct {
        bool m_bgWindowEnable : 1 = false;
        bool m_objEnable : 1 = false;
        bool m_objSize : 1 = false;
        bool m_bgTilemap : 1 = false;
        bool m_bgWindowTiles : 1 = false;
        bool m_windowEnable : 1 = false;
        bool m_windowTilemap : 1 = false;
        bool m_ppuEnable : 1 = false;
    } m_control;
    static_assert(sizeof(m_control) == 1);
    struct {
        uint8_t m_ppuMode : 2 = 2;
        bool m_lyc_is_ly : 1 = false;
        bool m_mode0InterruptSelect : 1 = false;
        bool m_mode1InterruptSelect : 1 = false;
        bool m_mode2InterruptSelect : 1 = false;
        bool m_lycInterruptSelect : 1 = false;
    } m_status;
    static_assert(sizeof(m_status) == 1);
    uint8_t m_scy = 0; // scy
    uint8_t m_scx = 0; // scx
    uint8_t m_ly = 0;  // lcd y
    uint8_t m_lyc = 0;
    uint8_t m_dma = 0;
    uint8_t m_bgp = 0;
    uint8_t m_obp0 = 0;
    uint8_t m_obp1 = 0;
    uint8_t m_windowY = 0;
    uint8_t m_windowX = 0; // plus 7 whatever that means
};
static_assert(sizeof(LCDRegisters) == 12);

struct InterruptControl {
  union {
    struct {
      bool vblank : 1 ;
      bool lcd : 1 ;
      bool timer : 1 ;
      bool serial : 1 ;
      bool joypad : 1 ;
      uint8_t detail_unused : 3 ;
    };
    uint8_t data = 0;
  };
};
static_assert(sizeof(InterruptControl) == sizeof(uint8_t));

struct alignas(uint8_t) IORegisters {
    uint8_t m_joypad{};
    uint8_t m_serialData;
    uint8_t m_serialControl;
    uint8_t detail_padding0{};
    uint8_t m_timerDivider{};
    uint8_t m_timerCounter{};
    uint8_t m_timerModulo{};
    uint8_t m_timerControl{};
    uint8_t detail_padding1[7]{};
    InterruptControl m_if{}; // interrupt flag
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
    uint8_t detail_padding6[5]{};
    uint8_t m_pcm12{};
    uint8_t m_pcm34{};
    uint8_t detail_padding7[8]{};
    uint8_t m_hram[127]{};
    InterruptControl m_ie{}; // interrupt enable
};

enum class Interrupts { VBLANK = 0, LCD = 1, TIMER = 2, SERIAL = 3, JOYPAD = 4 };

class IO {

  public:
    friend class Tester;
    friend class PPU;

    static constexpr iRange ADDRESS_RANGE = {0xFF00, 0x10000};
    static constexpr iRange HRAM_RANGE = {0xFF80, 0xFFFF};

    bool isBootromMapped() const { return !m_reg.m_bootromDisabled; };
    void setBootromMapped(bool val) { m_reg.m_bootromDisabled = !val; }

    void writeAddr(uint16_t addr, uint8_t val);
    void writeAddr16(uint16_t addr, uint16_t val);

    uint8_t readAddr(uint16_t address) const;
    uint16_t readAddr16(uint16_t address) const;

    LCDRegisters& getLCDRegisters() { return m_reg.m_lcd; }

    const std::string& getSerialOutput() { return m_serialOutput; }

    void setInterruptFlag(Interrupts f);

    void tick();

  private:
    // for testing only
    uint8_t* getMemPtrRW(uint16_t address);

    IORegisters m_reg;
    static_assert(sizeof(m_reg) == ADDRESS_RANGE.width());

    std::string m_serialOutput;
};
} // namespace ez