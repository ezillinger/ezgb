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

struct alignas(uint8_t) ObjectAttribute {
    uint8_t m_yPosMinus16 = 0;
    uint8_t m_xPosMinus8 = 0;
    uint8_t m_tileIdx = 0;
    struct  {
        uint8_t m_cgbPalette : 3 = 0;
        bool m_cgbBank : 1 = false;
        bool m_palette : 1 = false;
        bool m_flipX : 1 = false;
        bool m_flipY : 1 = false;
        bool m_priority : 1 = false;
    } m_attributes;
};
static_assert(sizeof(ObjectAttribute) == 4);

class PPU {

  public:
    friend class Tester;
    static constexpr int DISPLAY_WIDTH = 160;
    static constexpr int DISPLAY_HEIGHT = 144;
    static constexpr int BG_WINDOW_DIM_XY = 256;
    static constexpr int TILE_DIM_XY = 8;
    static constexpr int BYTES_PER_TILE_COMPRESSED = 16;
    static constexpr iRange VRAM_ADDR_RANGE = {0x8000, 0xA000};
    static constexpr iRange OAM_ADDR_RANGE = {0xFE00, 0xFEA0};

    static constexpr int VRAM_DEBUG_FB_WIDTH = 16 * TILE_DIM_XY;
    static constexpr int VRAM_DEBUG_FB_HEIGHT = 24 * TILE_DIM_XY;

    static constexpr int OAM_SPRITE_COUNT = OAM_ADDR_RANGE.width() / int(sizeof(ObjectAttribute));

    PPU(IORegisters& io);

    void tick();

    uint8_t readAddr(uint16_t) const;
    void writeAddr(uint16_t, uint8_t);

    const rgba8* getDisplayFramebuffer() const;

    // for debug only
    const rgba8* getWindowDebugFramebuffer();
    const rgba8* getBgDebugFramebuffer();
    const rgba8* getVramDebugFramebuffer();

    void reset();

  private:
    rgba8 getBGColor(const uint8_t paletteIdx) const;
    rgba8 getColor(const uint8_t paletteIdx) const;

    bool isVramAvailToCPU() const;
    bool isOamAvailToCPU() const;
    void setStatIRQHigh(StatIRQSources src);
    void updateLyLyc();
    void updateScanline();

    static void renderTile(const uint8_t* tileBegin, uint8_t* dst, int rowPitch);

    std::vector<uint8_t> renderOAM();
    void renderBgWindowRow(int tileY, bool enable, bool tileMap, uint8_t* dst);

    void updateBgRow(int line);
    void updateWindowRow(int line);

    int m_currentLineDotTickCount = 0;

    std::array<bool, +StatIRQSources::NUM_SOURCES> m_statIRQSources{};
    bool m_statIRQ = false;
    bool m_statIRQRisingEdge = false;

    IORegisters& m_reg;

    std::vector<uint8_t> m_bg = std::vector<uint8_t>(BG_WINDOW_DIM_XY * BG_WINDOW_DIM_XY);
    std::vector<uint8_t> m_window = std::vector<uint8_t>(BG_WINDOW_DIM_XY * BG_WINDOW_DIM_XY);

    std::vector<uint8_t> m_vram = std::vector<uint8_t>(VRAM_ADDR_RANGE.width());
    std::vector<uint8_t> m_oam = std::vector<uint8_t>(OAM_ADDR_RANGE.width());

    std::vector<rgba8> m_display = std::vector<rgba8>(size_t(DISPLAY_WIDTH * DISPLAY_HEIGHT));

    std::vector<rgba8> m_windowDebugFramebuffer = std::vector<rgba8>(size_t(BG_WINDOW_DIM_XY * BG_WINDOW_DIM_XY));
    std::vector<rgba8> m_bgDebugFramebuffer = std::vector<rgba8>(size_t(BG_WINDOW_DIM_XY * BG_WINDOW_DIM_XY));
    std::vector<rgba8> m_vramDebugFramebuffer = std::vector<rgba8>(VRAM_DEBUG_FB_HEIGHT * VRAM_DEBUG_FB_WIDTH);

    // whiter white than normal white
    const std::vector<rgba8> m_displayOff = std::vector<rgba8>(size_t(DISPLAY_WIDTH * DISPLAY_HEIGHT), rgba8{0xf2, 0xf4, 0xf9, 0xFF});
};
} // namespace ez