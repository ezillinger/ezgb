#pragma once
#include "Base.h"
#include "IO.h"

namespace ez {

enum class StatIRQSources {
    MODE_0 = 0,
    MODE_1,
    MODE_2,
    LY,
    NUM_SOURCES
};

class PPU {

  public:
    friend class Tester;
    static constexpr int DISPLAY_WIDTH = 160;
    static constexpr int DISPLAY_HEIGHT = 144;
    static constexpr int BG_DIM_XY = 256;
    static constexpr iRange VRAM_ADDR_RANGE = {0x8000, 0xA000};
    static constexpr iRange OAM_ADDR_RANGE = {0xFE00, 0xFEA0};

    PPU(IO& io);

    void tick();

    uint8_t readAddr(uint16_t) const;
    uint16_t readAddr16(uint16_t) const;

    void writeAddr(uint16_t, uint8_t);
    void writeAddr16(uint16_t, uint16_t);

    const rgba8* getDisplayFramebuffer() const;

  private:
    rgba8 getBGColor(const uint8_t paletteIdx);

    bool isVramAvailToCPU() const;
    bool isOamAvailToCPU() const;
    void setStatIRQHigh(StatIRQSources src);
    void updateLyLyc();
    void updateDisplay();

    static std::array<uint8_t, 64> renderTile(const uint8_t* tileBegin);

    void renderBGWindow(bool enable, bool tileMap, uint8_t* dst);
    void updateBG();
    void updateWindow();

    int m_currentLineDotTickCount = 0;

    std::array<bool, +StatIRQSources::NUM_SOURCES> m_statIRQSources{};
    bool m_statIRQ = false;
    bool m_statIRQRisingEdge = false;

    IO& m_io;
    IORegisters& m_reg;

    std::vector<uint8_t> m_bg = std::vector<uint8_t>(BG_DIM_XY * BG_DIM_XY);
    std::vector<uint8_t> m_window = std::vector<uint8_t>(BG_DIM_XY * BG_DIM_XY);

    std::vector<uint8_t> m_vram = std::vector<uint8_t>(VRAM_ADDR_RANGE.width(), 0u);
    std::vector<uint8_t> m_oam = std::vector<uint8_t>(OAM_ADDR_RANGE.width(), 0u);
    std::vector<rgba8> m_display = std::vector<rgba8>(size_t(DISPLAY_WIDTH * DISPLAY_HEIGHT));
};
} // namespace ez