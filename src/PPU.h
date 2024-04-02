#pragma once
#include "Base.h"

namespace ez {

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
    uint8_t m_ly = 0; // lcd y
    uint8_t m_lyc = 0;
    uint8_t m_dma = 0;
    uint8_t m_bgp = 0;
    uint8_t m_obp0 = 0;
    uint8_t m_obp1 = 0;
    uint8_t m_windowY = 0;
    uint8_t m_windowX = 0; // plus 7 whatever that means
};
static_assert(sizeof(LCDRegisters) == 12);

class PPU {

  public:
    static constexpr size_t VRAM_BYTES = 8 * 1024;
    static constexpr uint16_t VRAM_BASE_ADDR = 0x8000;

    PPU(LCDRegisters& reg);

    void tick();

    uint8_t* getMemPtrRW(uint16_t address);
    const uint8_t* getMemPtr(uint16_t address) const;


  private:

    int m_currentLineDotTickCount = 0;

    LCDRegisters& m_reg;
    std::vector<uint8_t> m_vram = std::vector<uint8_t>(VRAM_BYTES, 0u);
};
} // namespace ez