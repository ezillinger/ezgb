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

PPU::PPU(IO& io) : m_io(io), m_reg(io.getRegisters()) {}

void PPU::writeAddr(uint16_t addr, uint8_t data) {
    const auto offset = addr - VRAM_BASE_ADDR;
    EZ_ENSURE(size_t(offset) < VRAM_BYTES);
    m_vram[offset] = data;
}

void PPU::writeAddr16(uint16_t addr, uint16_t data) {
    const auto offset = addr - VRAM_BASE_ADDR;
    EZ_ENSURE(size_t(offset) < VRAM_BYTES);
    *reinterpret_cast<uint16_t*>(m_vram.data() + offset) = data;
}

uint8_t PPU::readAddr(uint16_t addr) const {
    const auto offset = addr - VRAM_BASE_ADDR;
    EZ_ENSURE(size_t(offset) < VRAM_BYTES);
    return m_vram[offset];
}

uint16_t PPU::readAddr16(uint16_t addr) const {
    const auto offset = addr - VRAM_BASE_ADDR;
    EZ_ENSURE(size_t(offset) < VRAM_BYTES);
    return *reinterpret_cast<const uint16_t*>(m_vram.data() + offset);
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
                    m_io.setInterruptFlag(Interrupts::VBLANK);
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
    for(const auto src : m_statIRQSources){
        statIRQ |= src;
    }
    if(update(m_statIRQ, statIRQ) && statIRQ){
        log_info("Stat IRQ fired!");
        m_io.setInterruptFlag(Interrupts::LCD);
    }
}


void PPU::updateDisplay() {
    static constexpr auto mid_x = DISPLAY_WIDTH / 2;
    static constexpr auto mid_y = DISPLAY_HEIGHT / 2;
    static int radiusStep = 0;
    radiusStep = (radiusStep + 1) % int(DISPLAY_WIDTH * 1.5f);
    static constexpr auto lineWidth = 2.0f;
    for (auto y = 0; y < DISPLAY_HEIGHT; ++y) {
        for(auto x = 0; x < DISPLAY_WIDTH; ++x){
            const auto dx = fabs(mid_x - x);
            const auto dy = fabs(mid_y - y);
            const auto radius = sqrtf(dx * dx + dy * dy);
            const uint8_t intensity = fabs(radius - radiusStep) < lineWidth ? 0xFF : 0;
            m_display[y * DISPLAY_WIDTH + x] = {intensity, intensity, intensity, 0xFF};
        }
    }
}

const rgba8* PPU::getDisplayFramebuffer() const { 
    return m_display.data(); 
}

void PPU::setStatIRQHigh(StatIRQSources src) { m_statIRQSources[+src] = true; }

void PPU::updateLyLyc() { 
    if(m_reg.m_lcd.m_ly == m_reg.m_lcd.m_lyc){
        m_reg.m_lcd.m_status.m_lyc_is_ly = true;
        if(m_reg.m_ie.lcd && m_reg.m_lcd.m_status.m_lycInterruptSelect){
            setStatIRQHigh(StatIRQSources::LY);
        }
    }
    else{
        m_reg.m_lcd.m_status.m_lyc_is_ly = false;
    }
}
} // namespace ez