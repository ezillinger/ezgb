#include "PPU.h"
#include "MiscOps.h"

namespace ez {

PPU::PPU(IORegisters& ioReg) : m_reg(ioReg) { reset(); }

void PPU::write_addr(uint16_t addr, uint8_t data) {
    if (VRAM_ADDR_RANGE.containsExclusive(addr)) {
        if (!is_vram_avail_to_cpu()) {
            log_warn("CPU write while VRAM inaccessible");
            return;
        }
        const auto offset = addr - VRAM_ADDR_RANGE.m_min;
        EZ_ENSURE(size_t(offset) < VRAM_ADDR_RANGE.width());
        m_vram[offset] = data;
    } else {
        EZ_ENSURE(OAM_ADDR_RANGE.containsExclusive(addr));
        if (!is_oam_avail_to_cpu()) {
            log_warn("CPU write while OAM inaccessible");
            return;
        }
        const auto offset = addr - OAM_ADDR_RANGE.m_min;
        EZ_ENSURE(size_t(offset) < OAM_ADDR_RANGE.width());
        m_oam[offset] = data;
    }
}

uint8_t PPU::read_addr(uint16_t addr) const {
    if (VRAM_ADDR_RANGE.containsExclusive(addr)) {
        if (!is_vram_avail_to_cpu()) {
            log_warn("CPU read while VRAM inaccessible");
            return 0xFF;
        }
        const auto offset = addr - VRAM_ADDR_RANGE.m_min;
        EZ_ENSURE(size_t(offset) < VRAM_ADDR_RANGE.width());
        return m_vram[offset];
    } else {
        if (!is_oam_avail_to_cpu()) {
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

    if (!m_reg.m_lcd.m_control.m_ppuEnable) {
        return;
    }

    for (auto& v : m_statIRQSources) {
        v = false;
    }

    ++m_currentLineDotTickCount;
    switch (m_reg.m_lcd.m_status.m_ppuMode) {
        case +PPUMode::OAM_SCAN:
            if (m_currentLineDotTickCount == 80) {
                do_oam_scan();
                m_currentLineDotTickCount = 0;
                m_reg.m_lcd.m_status.m_ppuMode = +PPUMode::DRAWING;
            }
            break;
        case +PPUMode::DRAWING:
            if (m_currentLineDotTickCount == 200) {
                m_reg.m_lcd.m_status.m_ppuMode = +PPUMode::HBLANK;
                if (m_reg.m_ie.lcd && m_reg.m_lcd.m_status.m_mode0InterruptSelect) {
                    set_stat_irq(StatIRQSources::MODE_0);
                }
            }
            break;
        case +PPUMode::HBLANK:
            ez_assert(m_reg.m_lcd.m_ly < DISPLAY_HEIGHT);
            if (m_currentLineDotTickCount == 456) {
                update_scanline();
                ++m_reg.m_lcd.m_ly;
                m_currentLineDotTickCount = 0;
                if (m_reg.m_lcd.m_ly == DISPLAY_HEIGHT) {
                    m_displayOnLastVBlank = m_display;
                    m_reg.m_lcd.m_status.m_ppuMode = +PPUMode::VBLANK;
                    m_reg.m_if.vblank = true;
                    if (m_reg.m_ie.lcd && m_reg.m_lcd.m_status.m_mode1InterruptSelect) {
                        set_stat_irq(StatIRQSources::MODE_1);
                    }
                }
                else{
                    m_reg.m_lcd.m_status.m_ppuMode = +PPUMode::OAM_SCAN;
                }
                update_ly_eq_lyc();
            }
            break;
        case +PPUMode::VBLANK:
            ez_assert(m_reg.m_lcd.m_ly >= DISPLAY_HEIGHT);
            ez_assert(m_reg.m_lcd.m_ly < DISPLAY_HEIGHT + 10);
            if (m_currentLineDotTickCount == 456) {
                ++m_reg.m_lcd.m_ly;
                m_currentLineDotTickCount = 0;
                if (m_reg.m_lcd.m_ly == 154) {
                    m_reg.m_lcd.m_ly = 0;
                    m_reg.m_lcd.m_status.m_ppuMode = +PPUMode::OAM_SCAN;
                    if (m_reg.m_ie.lcd && m_reg.m_lcd.m_status.m_mode2InterruptSelect) {
                        set_stat_irq(StatIRQSources::MODE_2);
                    }
                }
                update_ly_eq_lyc();
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

std::span<const rgba8> PPU::get_display_framebuffer() const {
    if (m_reg.m_lcd.m_control.m_ppuEnable) {
        // don't let display tear
        static constexpr bool allowTearing = false;
        if(allowTearing){
            return m_display;
        } else {
            return m_displayOnLastVBlank;
        }
    } else {
        return m_displayOff;
    }
}

std::span<const rgba8> PPU::get_window_dbg_framebuffer() {
    for (int i = 0; i < BG_WINDOW_DIM_XY / TILE_DIM_XY; ++i) {
        render_bg_window_row(i, true, m_reg.m_lcd.m_control.m_windowTilemap, m_window.data());
    }
    for (auto y = 0; y < BG_WINDOW_DIM_XY; ++y) {
        for (auto x = 0; x < BG_WINDOW_DIM_XY; ++x) {
            m_windowDebugFramebuffer[y * BG_WINDOW_DIM_XY + x] = get_bg_color(m_window[y * BG_WINDOW_DIM_XY + x]);
        }
    }
    return m_windowDebugFramebuffer;
}

std::span<const rgba8> PPU::get_bg_dbg_framebuffer() {
    for (int i = 0; i < BG_WINDOW_DIM_XY / TILE_DIM_XY; ++i) {
        render_bg_window_row(i, true, m_reg.m_lcd.m_control.m_bgTilemap, m_bg.data());
    }
    for (auto y = 0; y < BG_WINDOW_DIM_XY; ++y) {
        for (auto x = 0; x < BG_WINDOW_DIM_XY; ++x) {
            m_bgDebugFramebuffer[y * BG_WINDOW_DIM_XY + x] = get_bg_color(m_bg[y * BG_WINDOW_DIM_XY + x]);
        }
    }
    return m_bgDebugFramebuffer;
}

std::span<const rgba8> PPU::get_vram_dbg_framebuffer() {
    auto tile = std::vector<uint8_t>(TILE_DIM_XY * TILE_DIM_XY);
    for (auto i = 0; i < 384; ++i) {
        render_tile(m_vram.data() + i * BYTES_PER_TILE_COMPRESSED, tile.data(), TILE_DIM_XY);
        const auto dstRow = i / 16;
        const auto dstCol = i % 16;
        const auto bytesPerRow = 16 * TILE_DIM_XY * TILE_DIM_XY;
        for (auto ty = 0; ty < TILE_DIM_XY; ++ty) {
            const auto dstOffset = (dstRow * bytesPerRow + ty * TILE_DIM_XY * 16) + dstCol * TILE_DIM_XY;
            for (auto tx = 0; tx < TILE_DIM_XY; ++tx) {
                m_vramDebugFramebuffer[dstOffset + tx] = get_color(tile[ty * TILE_DIM_XY + tx]);
            }
        }
    }
    return m_vramDebugFramebuffer;
}

void PPU::set_stat_irq(StatIRQSources src) { m_statIRQSources[+src] = true; }

void PPU::render_tile(const uint8_t* tileBegin, uint8_t* dst, int rowPitch) {
    for (auto y = 0; y < TILE_DIM_XY; ++y) {
        const auto byte0 = tileBegin[(y * 2)];
        const auto byte1 = tileBegin[(y * 2) + 1];
        for (auto x = 0; x < TILE_DIM_XY; ++x) {
            const uint8_t bit0 = byte0 & (0b1000'0000 >> x) ? 0b1 : 0;
            const uint8_t bit1 = byte1 & (0b1000'0000 >> x) ? 0b1 : 0;
            const uint8_t px = bit1 << 1 | bit0;
            ez_assert(px < 4);
            dst[y * rowPitch + x] = px;
        }
    }
}

void PPU::update_scanline() {

    const auto y = m_reg.m_lcd.m_ly;
    const auto objHeight = m_reg.m_lcd.m_control.m_objSize ? 16 : 8;
    
    // todo, don't draw the entire bg/window every line
    update_bg_row(y);
    update_window_row(y);

    auto renderedTile = std::vector<uint8_t>(TILE_DIM_XY * TILE_DIM_XY);
    for (auto x = 0; x < DISPLAY_WIDTH; ++x) {

        const auto bgY = (y + m_reg.m_lcd.m_scy) % BG_WINDOW_DIM_XY;
        const auto bgX = (x + m_reg.m_lcd.m_scx) % BG_WINDOW_DIM_XY;

        const auto windowLeft = m_reg.m_lcd.m_windowXPlus7 - 7;
        const auto windowTop = m_reg.m_lcd.m_windowY;
        const auto wX = x - windowLeft;
        const auto wY = y - windowTop;

        const bool inWindow = m_reg.m_lcd.m_control.m_windowEnable &&
                              iRange{windowLeft, windowLeft + BG_WINDOW_DIM_XY}.containsExclusive(x) &&
                              iRange{windowTop, windowTop + BG_WINDOW_DIM_XY}.containsExclusive(y);

        auto bgPaletteIdx = inWindow ? m_window[wY * BG_WINDOW_DIM_XY + wX] : m_bg[bgY * BG_WINDOW_DIM_XY + bgX];
        const auto bgColorIdx = sample_palette(bgPaletteIdx, m_reg.m_lcd.m_bgp);
        uint8_t spritePaletteIdx = 0;
        auto spritePriority = false;
        auto spritePalette = false;
        for (const auto& spritePair : m_spritesAndOamIdxOnLine) {
            const auto& sprite = spritePair.first;
            if (!iRange(sprite.m_xPosMinus8 - 8, sprite.m_xPosMinus8).containsExclusive(x)) {
                continue;
            }
            auto spriteY = y - (sprite.m_yPosMinus16 - 16);
            if (sprite.m_attributes.m_flipY) {
                spriteY = objHeight - 1 - spriteY;
            }
            ez_assert(spriteY < 16);
            auto spriteX = x - (sprite.m_xPosMinus8 - 8);
            if (sprite.m_attributes.m_flipX) {
                spriteX = TILE_DIM_XY - 1 - spriteX;
            }
            ez_assert(spriteX < 8);
            auto tileIdx = sprite.m_tileIdx;
            const bool isTopTile = spriteY < 8;
            if (isTopTile && m_reg.m_lcd.m_control.m_objSize) {
                tileIdx &= 0xFE;
            } else if(!isTopTile){
                tileIdx |= 0x01;
            }
            render_tile(m_vram.data() + tileIdx * BYTES_PER_TILE_COMPRESSED, renderedTile.data(), TILE_DIM_XY);
            spritePaletteIdx = renderedTile[(spriteY % TILE_DIM_XY) * TILE_DIM_XY + spriteX];
            spritePriority = sprite.m_attributes.m_priority;
            spritePalette = sprite.m_attributes.m_palette;
            ez_assert(spritePaletteIdx < 4);
            if (spritePaletteIdx) {
                break;
            }
        }
        const bool useSpritePx =
            (spritePaletteIdx != 0) && (spritePriority ? bgColorIdx == 0 : true);

        const auto spriteColorIdx = sample_palette(
            spritePaletteIdx, spritePalette ? m_reg.m_lcd.m_obp1 : m_reg.m_lcd.m_obp0);
        const auto color = get_color(useSpritePx ? spriteColorIdx : bgColorIdx);
        m_display[y * DISPLAY_WIDTH + x] = color;
    }
}

void PPU::do_oam_scan() {

    m_spritesAndOamIdxOnLine.clear();

    const auto y = m_reg.m_lcd.m_ly;
    const auto objHeight = m_reg.m_lcd.m_control.m_objSize ? 16 : 8;
    // todo, move this to OAM scan
    if (m_reg.m_lcd.m_control.m_objEnable) {
        ez_assert(iRange{0, DISPLAY_HEIGHT}.containsExclusive(y));
        for (auto objIdx = 0; objIdx < OAM_SPRITE_COUNT; ++objIdx) {
            const auto oa = reinterpret_cast<ObjectAttribute*>(m_oam.data())[objIdx];
            const auto yMin = oa.m_yPosMinus16 - 16;
            const auto spriteRangeY = iRange{yMin, yMin + objHeight};
            if (spriteRangeY.containsExclusive(y)) {
                m_spritesAndOamIdxOnLine.push_back({oa, objIdx});
                if (m_spritesAndOamIdxOnLine.size() == 10) {
                    break;
                }
            }
        }
        ez_assert(m_spritesAndOamIdxOnLine.size() <= 10);
    }

    std::sort(m_spritesAndOamIdxOnLine.begin(), m_spritesAndOamIdxOnLine.end(),
              [](const ObjAndIdx& lhs, const ObjAndIdx& rhs) {
                  return lhs.first.m_xPosMinus8 == rhs.first.m_xPosMinus8
                             ? lhs.second < rhs.second
                             : lhs.first.m_xPosMinus8 < rhs.first.m_xPosMinus8;
              });
}

void PPU::update_bg_row(int y) {
    const auto bgY = (y + m_reg.m_lcd.m_scy) % BG_WINDOW_DIM_XY;
    const auto tileY = bgY / TILE_DIM_XY;

    render_bg_window_row(tileY, m_reg.m_lcd.m_control.m_bgWindowEnable,
                      m_reg.m_lcd.m_control.m_bgTilemap, m_bg.data());
}

void PPU::update_window_row(int y) {

    const auto windowTop = m_reg.m_lcd.m_windowY;
    const auto windowY = (BG_WINDOW_DIM_XY + y - windowTop) % BG_WINDOW_DIM_XY;
    const auto tileY = windowY / TILE_DIM_XY;

    render_bg_window_row(
        tileY, m_reg.m_lcd.m_control.m_windowEnable && m_reg.m_lcd.m_control.m_bgWindowEnable,
        m_reg.m_lcd.m_control.m_windowTilemap, m_window.data());
}

void PPU::render_bg_window_row(int tileY, bool enable, bool tileMap, uint8_t* dst) {
    ez_assert(tileY >= 0);
    if (!enable) {
        memset(dst + (tileY * BG_WINDOW_DIM_XY), 0, BG_WINDOW_DIM_XY * TILE_DIM_XY);
        return;
    }

    const auto tileMapOffset = (tileMap ? 0x9C00 : 0x9800) - VRAM_ADDR_RANGE.m_min;

    const auto getTilePtr = [&](uint8_t tileIdx) {
        if (m_reg.m_lcd.m_control.m_bgWindowTileAddrMode) {
            const auto offset = (0x8000 - VRAM_ADDR_RANGE.m_min) + (tileIdx * BYTES_PER_TILE_COMPRESSED);
            return m_vram.data() + offset;
        } else {
            const int signedIdx = static_cast<int8_t>(tileIdx);
            const auto offset = (0x9000 - VRAM_ADDR_RANGE.m_min) + (signedIdx * BYTES_PER_TILE_COMPRESSED);
            return m_vram.data() + offset;
        }
    };
    const auto tilesPerRow = BG_WINDOW_DIM_XY / TILE_DIM_XY;
    auto renderedTile = std::vector<uint8_t>(TILE_DIM_XY * TILE_DIM_XY);
    for (auto tileX = 0; tileX < tilesPerRow; ++tileX) {
        const auto tileIdx = m_vram[tileMapOffset + (tileY * tilesPerRow) + tileX];
        const auto tilePtr = getTilePtr(tileIdx);
        render_tile(tilePtr, renderedTile.data(), 8);
        for (auto tilePxY = 0; tilePxY < TILE_DIM_XY; ++tilePxY) {
            auto dstPtr = dst + ((tileY * TILE_DIM_XY + tilePxY) * BG_WINDOW_DIM_XY) + tileX * TILE_DIM_XY;
            for (auto tilePxX = 0; tilePxX < TILE_DIM_XY; ++tilePxX) {
                dstPtr[tilePxX] = renderedTile[tilePxY * TILE_DIM_XY + tilePxX];
            }
        }
    }
}

bool PPU::is_vram_avail_to_cpu() const {
    if (m_reg.m_lcd.m_control.m_ppuEnable == false) {
        return true;
    }
    return m_reg.m_lcd.m_status.m_ppuMode != +PPUMode::DRAWING;
}

void PPU::reset() {
    // todo, verify this is all the resets when LCD is disabled
    log_warn("PPU Reset!");
    m_currentLineDotTickCount = 0;
    m_reg.m_lcd.m_ly = 0;
    m_reg.m_lcd.m_status.m_ppuMode = 0;
    m_statIRQ = false;
    m_statIRQRisingEdge = false;

    // todo, fix gcc complains about memset
    for (auto& px : m_display) {
        px = {};
    }
}

bool PPU::is_oam_avail_to_cpu() const {
    if (m_reg.m_lcd.m_control.m_ppuEnable == false) {
        return true;
    }
    return m_reg.m_lcd.m_status.m_ppuMode == +PPUMode::VBLANK ||
           m_reg.m_lcd.m_status.m_ppuMode == +PPUMode::HBLANK;
}

rgba8 PPU::get_bg_color(const uint8_t paletteIdx) const {
    ez_assert(paletteIdx < 4);

    const auto bgp = m_reg.m_lcd.m_bgp;
    const uint8_t colorIdx = ((0b11 << (2 * paletteIdx)) & bgp) >> (2 * paletteIdx);
    return get_color(colorIdx);
}

rgba8 PPU::get_color(const uint8_t colorIdx) const {
    static constexpr std::array<rgba8, 4> colors{
        rgba8{0xb2, 0xb4, 0xb9, 0xFF},
        rgba8{0x60, 0x96, 0x9f, 0xFF},
        rgba8{0x26, 0x5a, 0x37, 0xFF},
        rgba8{0x3d, 0x2e, 0x00, 0xFF},
    };

    ez_assert(colorIdx < 4);
    return colors[colorIdx];
}

void PPU::update_ly_eq_lyc() {
    if (m_reg.m_lcd.m_ly == m_reg.m_lcd.m_lyc) {
        m_reg.m_lcd.m_status.m_lycIsLy = true;
        if (m_reg.m_ie.lcd && m_reg.m_lcd.m_status.m_lycInterruptSelect) {
            set_stat_irq(StatIRQSources::LY);
        }
    } else {
        m_reg.m_lcd.m_status.m_lycIsLy = false;
    }
}
} // namespace ez