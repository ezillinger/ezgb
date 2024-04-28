#include "PPU.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#if EZ_GCC
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include "libs/stb_image_write.h"
#if EZ_GCC
    #pragma GCC diagnostic pop
#endif

namespace ez {

PPU::PPU(IORegisters& ioReg) : m_reg(ioReg) {}

void PPU::writeAddr(uint16_t addr, uint8_t data) {
    if (VRAM_ADDR_RANGE.containsExclusive(addr)) {
        if (!isVramAvailToCPU()) {
            log_warn("CPU write while VRAM inaccessible");
            return;
        }
        const auto offset = addr - VRAM_ADDR_RANGE.m_min;
        EZ_ENSURE(size_t(offset) < VRAM_ADDR_RANGE.width());
        m_vram[offset] = data;
    } else {
        EZ_ENSURE(OAM_ADDR_RANGE.containsExclusive(addr));
        if (!isOamAvailToCPU()) {
            log_warn("CPU write while OAM inaccessible");
            return;
        }
        const auto offset = addr - OAM_ADDR_RANGE.m_min;
        EZ_ENSURE(size_t(offset) < OAM_ADDR_RANGE.width());
        m_oam[offset] = data;
    }
}

uint8_t PPU::readAddr(uint16_t addr) const {
    if (VRAM_ADDR_RANGE.containsExclusive(addr)) {
        if (!isVramAvailToCPU()) {
            log_warn("CPU read while VRAM inaccessible");
            return 0xFF;
        }
        const auto offset = addr - VRAM_ADDR_RANGE.m_min;
        EZ_ENSURE(size_t(offset) < VRAM_ADDR_RANGE.width());
        return m_vram[offset];
    } else {
        if (!isOamAvailToCPU()) {
            log_warn("CPU read while OAM inaccessible");
            return 0xFF;
        }
        EZ_ENSURE(OAM_ADDR_RANGE.containsExclusive(addr));
        const auto offset = addr - OAM_ADDR_RANGE.m_min;
        EZ_ENSURE(size_t(offset) < OAM_ADDR_RANGE.width());
        return m_oam[offset];
    }
}

void PPU::tick() {

    for (auto& v : m_statIRQSources) {
        v = false;
    }

    ++m_currentLineDotTickCount;
    switch (m_reg.m_lcd.m_status.m_ppuMode) {
        case +PPUMode::OAM_SCAN:
            if (m_currentLineDotTickCount == 80) {
                m_currentLineDotTickCount = 0;
                m_reg.m_lcd.m_status.m_ppuMode = +PPUMode::DRAWING;
            }
            break;
        case +PPUMode::DRAWING:
            if (m_currentLineDotTickCount == 200) {
                m_reg.m_lcd.m_status.m_ppuMode = +PPUMode::HBLANK;
                if (m_reg.m_ie.lcd && m_reg.m_lcd.m_status.m_mode0InterruptSelect) {
                    setStatIRQHigh(StatIRQSources::MODE_0);
                }
            }
            break;
        case +PPUMode::HBLANK:
            if (m_currentLineDotTickCount == 456) {
                ++m_reg.m_lcd.m_ly;
                updateLyLyc();
                m_currentLineDotTickCount = 0;
                if (m_reg.m_lcd.m_ly == 144) {
                    m_reg.m_lcd.m_status.m_ppuMode = +PPUMode::VBLANK;
                    m_reg.m_if.vblank = true;
                    if (m_reg.m_ie.lcd && m_reg.m_lcd.m_status.m_mode1InterruptSelect) {
                        setStatIRQHigh(StatIRQSources::MODE_1);
                    }
                }
            }
            break;
        case +PPUMode::VBLANK:
            if (m_currentLineDotTickCount == 456) {
                ++m_reg.m_lcd.m_ly;
                updateLyLyc();
                m_currentLineDotTickCount = 0;
                if (m_reg.m_lcd.m_ly == 154) {
                    m_reg.m_lcd.m_ly = 0;
                    updateLyLyc();
                    m_reg.m_lcd.m_status.m_ppuMode = +PPUMode::OAM_SCAN;
                    if (m_reg.m_ie.lcd && m_reg.m_lcd.m_status.m_mode2InterruptSelect) {
                        setStatIRQHigh(StatIRQSources::MODE_2);
                    }
                    updateDisplay();
                }
            }
            break;
    }

    // stat IRQ only fires on rising edge, if it never falls it never fires
    auto statIRQ = false;
    for (const auto src : m_statIRQSources) {
        statIRQ |= src;
    }
    if (update(m_statIRQ, statIRQ) && statIRQ) {
        m_reg.m_if.lcd = true;
    }
}

void PPU::updateDisplay() {
    updateBG();
    updateWindow();
    for (auto y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (auto x = 0; x < DISPLAY_WIDTH; ++x) {

            const auto bgY = (y + m_reg.m_lcd.m_scy) % BG_DIM_XY;
            const auto bgX = (x + m_reg.m_lcd.m_scx) % BG_DIM_XY;

            const auto windowLeft = m_reg.m_lcd.m_windowXPlus7 - 7;
            const auto windowTop = m_reg.m_lcd.m_windowY;
            const auto wX = x - windowLeft;
            const auto wY = y - windowTop;

            const bool inWindow = m_reg.m_lcd.m_control.m_windowEnable &&
                                  iRange{windowLeft, windowLeft + BG_DIM_XY}.containsExclusive(x) &&
                                  iRange{windowTop, windowTop + BG_DIM_XY}.containsExclusive(y);

            uint8_t px = inWindow ? m_window[wY * BG_DIM_XY + wX] : m_bg[bgY * BG_DIM_XY + bgX];
            m_display[y * DISPLAY_WIDTH + x] = getBGColor(px);
        }
    }
}

const rgba8* PPU::getDisplayFramebuffer() const { return m_display.data(); }

void PPU::setStatIRQHigh(StatIRQSources src) { m_statIRQSources[+src] = true; }

std::array<uint8_t, 64> PPU::renderTile(const uint8_t* tileBegin) {
    auto result = std::array<uint8_t, 64>();
    const auto tileDim = 8;
    for (auto y = 0; y < tileDim; ++y) {
        const auto byte0 = tileBegin[(y * 2)];
        const auto byte1 = tileBegin[(y * 2) + 1];
        for (auto x = 0; x < tileDim; ++x) {
            const auto bit0 = byte0 & (0b1000'0000 >> x) ? 0b1 : 0;
            const auto bit1 = byte1 & (0b1000'0000 >> x) ? 0b1 : 0;
            const auto px = bit1 << 1 | bit0;
            result[y * tileDim + x] = px;
        }
    }

    return result;
}

void PPU::renderBGWindow(bool enable, bool tileMap, uint8_t* dst) {
    if (!enable) {
        memset(dst, 0, BG_DIM_XY * BG_DIM_XY);
        return;
    }

    const auto tileMapOffset = (tileMap ? 0x9C00 : 0x9800) - VRAM_ADDR_RANGE.m_min;
    ;

    const auto tileBytes = 16;
    const auto getTilePtr = [&](uint8_t tileIdx) {
        if (m_reg.m_lcd.m_control.m_bgWindowTileAddrMode) {
            const auto offset = (0x8000 - VRAM_ADDR_RANGE.m_min) + (tileIdx * tileBytes);
            return m_vram.data() + offset;
        } else {
            const auto signedIdx = static_cast<int8_t>(tileIdx);
            const auto offset = (0x9000 - VRAM_ADDR_RANGE.m_min) + (signedIdx * tileBytes);
            return m_vram.data() + offset;
        }
    };
    const auto tileMapDim = 32;
    const auto tileDim = 8;
    for (auto tmy = 0; tmy < tileMapDim; ++tmy) {
        for (auto tmx = 0; tmx < tileMapDim; ++tmx) {
            const auto tileIdx = m_vram[tileMapOffset + (tmy * tileMapDim) + tmx];
            const auto tilePtr = getTilePtr(tileIdx);
            const auto renderedTile = renderTile(tilePtr);
            for (auto ty = 0; ty < tileDim; ++ty) {
                auto dstPtr = dst + ((tmy * tileDim + ty) * BG_DIM_XY) + tmx * tileDim;
                for (auto tx = 0; tx < tileDim; ++tx) {
                    dstPtr[tx] = renderedTile[ty * tileDim + tx];
                }
            }
        }
    }
}

void PPU::updateWindow() {
    renderBGWindow(m_reg.m_lcd.m_control.m_windowEnable && m_reg.m_lcd.m_control.m_bgWindowEnable,
                   m_reg.m_lcd.m_control.m_windowTilemap, m_window.data());
}

void PPU::updateBG() {
    renderBGWindow(m_reg.m_lcd.m_control.m_bgWindowEnable, m_reg.m_lcd.m_control.m_bgTilemap,
                   m_bg.data());
}

bool PPU::isVramAvailToCPU() const {
    if (m_reg.m_lcd.m_control.m_ppuEnable == false) {
        return true;
    }
    return m_reg.m_lcd.m_status.m_ppuMode != +PPUMode::DRAWING;
}

bool PPU::isOamAvailToCPU() const {
    if (m_reg.m_lcd.m_control.m_ppuEnable == false) {
        return true;
    }
    return m_reg.m_lcd.m_status.m_ppuMode == +PPUMode::VBLANK ||
           m_reg.m_lcd.m_status.m_ppuMode == +PPUMode::HBLANK;
}

rgba8 PPU::getBGColor(const uint8_t paletteIdx) {
    EZ_ASSERT(paletteIdx < 4);
    static constexpr std::array<rgba8, 4> colors{
        rgba8{0xe2, 0xe4, 0xe9, 0xFF},
        rgba8{0x60, 0x96, 0x9f, 0xFF},
        rgba8{0x26, 0x5a, 0x37, 0xFF},
        rgba8{0x3d, 0x2e, 0x00, 0xFF},
    };

    const auto bgp = m_reg.m_lcd.m_bgp;
    const auto colorIdx = ((0b11 << (2 * paletteIdx)) & bgp) >> (2 * paletteIdx);
    EZ_ASSERT(colorIdx < 4);

    return colors[colorIdx];
}

void PPU::updateLyLyc() {
    if (m_reg.m_lcd.m_ly == m_reg.m_lcd.m_lyc) {
        m_reg.m_lcd.m_status.m_lyc_is_ly = true;
        if (m_reg.m_ie.lcd && m_reg.m_lcd.m_status.m_lycInterruptSelect) {
            setStatIRQHigh(StatIRQSources::LY);
        }
    } else {
        m_reg.m_lcd.m_status.m_lyc_is_ly = false;
    }
}
} // namespace ez