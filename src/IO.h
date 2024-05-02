#pragma once
#include "Base.h"

namespace ez {

// https://gbdev.io/pandocs/Hardware_Reg_List.html
enum class IOAddr {
    P1_JOYP = 0xFF00,      // Joypad	Mixed	All
    SB = 0xFF01,           // Serial transfer data	R/W	All
    SC = 0xFF02,           // Serial transfer control	R/W	Mixed
    DIV = 0xFF04,          // Divider register	R/W	All
    TIMA = 0xFF05,         // Timer counter	R/W	All
    TMA = 0xFF06,          // Timer modulo	R/W	All
    TAC = 0xFF07,          // Timer control	R/W	All
    IF = 0xFF0F,           // Interrupt flag	R/W	All
    NR10 = 0xFF10,         // Sound channel 1 sweep	R/W	All
    NR11 = 0xFF11,         // Sound channel 1 length timer & duty cycle	Mixed	All
    NR12 = 0xFF12,         // Sound channel 1 volume & envelope	R/W	All
    NR13 = 0xFF13,         // Sound channel 1 period low	W	All
    NR14 = 0xFF14,         // Sound channel 1 period high & control	Mixed	All
    NR21 = 0xFF16,         // Sound channel 2 length timer & duty cycle	Mixed	All
    NR22 = 0xFF17,         // Sound channel 2 volume & envelope	R/W	All
    NR23 = 0xFF18,         // Sound channel 2 period low	W	All
    NR24 = 0xFF19,         // Sound channel 2 period high & control	Mixed	All
    NR30 = 0xFF1A,         // Sound channel 3 DAC enable	R/W	All
    NR31 = 0xFF1B,         // Sound channel 3 length timer	W	All
    NR32 = 0xFF1C,         // Sound channel 3 output level	R/W	All
    NR33 = 0xFF1D,         // Sound channel 3 period low	W	All
    NR34 = 0xFF1E,         // Sound channel 3 period high & control	Mixed	All
    NR41 = 0xFF20,         // Sound channel 4 length timer	W	All
    NR42 = 0xFF21,         // Sound channel 4 volume & envelope	R/W	All
    NR43 = 0xFF22,         // Sound channel 4 frequency & randomness	R/W	All
    NR44 = 0xFF23,         // Sound channel 4 control	Mixed	All
    NR50 = 0xFF24,         // Master volume & VIN panning	R/W	All
    NR51 = 0xFF25,         // Sound panning	R/W	All
    NR52 = 0xFF26,         // Sound on/off	Mixed	All
    WaveRAMBegin = 0xFF30, // Storage for one of the sound channels’ waveform	R/W	All
    LCDC = 0xFF40,         // LCD control	R/W	All
    STAT = 0xFF41,         // LCD status	Mixed	All
    SCY = 0xFF42,          // Viewport Y position	R/W	All
    SCX = 0xFF43,          // Viewport X position	R/W	All
    LY = 0xFF44,           // LCD Y coordinate	R	All
    LYC = 0xFF45,          // LY compare	R/W	All
    DMA = 0xFF46,          // OAM DMA source address & start	R/W	All
    BGP = 0xFF47,          // BG palette data	R/W	DMG
    OBP0 = 0xFF48,         // OBJ palette 0 data	R/W	DMG
    OBP1 = 0xFF49,         // OBJ palette 1 data	R/W	DMG
    WY = 0xFF4A,           // Window Y position	R/W	All
    WX = 0xFF4B,           // Window X position plus 7	R/W	All
    KEY1 = 0xFF4D,         // Prepare speed switch	Mixed	CGB
    VBK = 0xFF4F,          // VRAM bank	R/W	CGB
    HDMA1 = 0xFF51,        // VRAM DMA source high	W	CGB
    HDMA2 = 0xFF52,        // VRAM DMA source low	W	CGB
    HDMA3 = 0xFF53,        // VRAM DMA destination high	W	CGB
    HDMA4 = 0xFF54,        // VRAM DMA destination low	W	CGB
    HDMA5 = 0xFF55,        // VRAM DMA length/mode/start	R/W	CGB
    RP = 0xFF56,           // Infrared communications port	Mixed	CGB
    BCPS_BGPI = 0xFF68,    // Background color palette spec / Background palette index R/W	CGB
    BCPD_BGPD = 0xFF69,    //	Background color palette data / Background palette data	R/W	CGB
    OCPS_OBPI = 0xFF6A,    //	OBJ color palette specification / OBJ palette index	R/W	CGB
    OCPD_OBPD = 0xFF6B,    //	OBJ color palette data / OBJ palette data	R/W	CGB
    OPRI = 0xFF6C,         //	Object priority mode	R/W	CGB
    SVBK = 0xFF70,         //	WRAM bank	R/W	CGB
    PCM12 = 0xFF76,        //	Audio digital outputs 1 & 2	R	CGB
    PCM34 = 0xFF77,        //	Audio digital outputs 3 & 4	R	CGB
    IE = 0xFFFF,           //	Interrupt enable	R/W	All
};

enum class PPUMode {
    OAM_SCAN = 2,
    DRAWING = 3,
    HBLANK = 0,
    VBLANK = 1
};

struct alignas(uint8_t) LCDRegisters {
    union {
        struct {
            bool m_bgWindowEnable : 1;
            bool m_objEnable : 1;
            bool m_objSize : 1; // 8x8 if false, 8x16 if true
            bool m_bgTilemap : 1;
            bool m_bgWindowTileAddrMode : 1;
            bool m_windowEnable : 1;
            bool m_windowTilemap : 1;
            bool m_ppuEnable : 1;
        };
        uint8_t m_data = 0;
    } m_control;
    static_assert(sizeof(m_control) == 1);
    union {
        struct {
            uint8_t m_ppuMode : 2;
            bool m_lyc_is_ly : 1;
            bool m_mode0InterruptSelect : 1;
            bool m_mode1InterruptSelect : 1;
            bool m_mode2InterruptSelect : 1;
            bool m_lycInterruptSelect : 1;
        };
        uint8_t m_data = 0;
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
    uint8_t m_windowXPlus7 = 0;
};
static_assert(sizeof(LCDRegisters) == 12);

struct InterruptControl {
    union {
        struct {
            bool vblank : 1;
            bool lcd : 1;
            bool timer : 1;
            bool serial : 1;
            bool joypad : 1;
            uint8_t detail_unused : 3;
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
    uint8_t m_tima{}; // timer ocunter
    uint8_t m_tma{};  // timer modula
    uint8_t m_tac{};  // timer control
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

static constexpr iRange WRAM0_ADDR_RANGE = {0xC000, 0xD000};
static constexpr iRange WRAM1_ADDR_RANGE = {0xD000, 0xE000};
static constexpr iRange MIRROR_ADDR_RANGE = {0xE000, 0xFE00};
static constexpr iRange IO_ADDR_RANGE = {0xFF00, 0x10000};
static_assert(sizeof(IORegisters) == IO_ADDR_RANGE.width());

static constexpr iRange HRAM_ADDR_RANGE = {0xFF80, 0xFFFF};

enum class Interrupts {
    VBLANK = 0,
    LCD = 1,
    TIMER = 2,
    SERIAL = 3,
    JOYPAD = 4,
    NUM_INTERRUPTS
};

struct JoypadState {
    bool m_a = false;
    bool m_b = false;
    bool m_start = false;
    bool m_select = false;
    bool m_left = false;
    bool m_right = false;
    bool m_up = false;
    bool m_down = false;
};

} // namespace ez