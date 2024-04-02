#include "PPU.h"

namespace ez {

PPU::PPU(LCDRegisters& reg) : m_reg(reg) {}

uint8_t* PPU::getMemPtrRW(uint16_t addr) {
    const auto offset = addr - VRAM_BASE_ADDR;
    EZ_ENSURE(size_t(offset) < VRAM_BYTES);
    return m_vram.data() + offset;
}

const uint8_t* PPU::getMemPtr(uint16_t addr) const {
    return const_cast<PPU*>(this)->getMemPtrRW(addr);
}

void PPU::tick() {
    ++m_currentLineDotTickCount;
    switch (m_reg.m_status.m_ppuMode) {
        case +PPUMode::OAM_SCAN:
            if (m_currentLineDotTickCount == 80) {
                m_currentLineDotTickCount = 0;
                m_reg.m_status.m_ppuMode = +PPUMode::DRAWING;
            }
            break;
        case +PPUMode::DRAWING:
            if (m_currentLineDotTickCount == 200) {
                m_reg.m_status.m_ppuMode = +PPUMode::HBLANK;
            }
            break;
        case +PPUMode::HBLANK:
            if (m_currentLineDotTickCount == 456) {
                ++m_reg.m_ly;
                m_currentLineDotTickCount = 0;
                if (m_reg.m_ly == 144) {
                    m_reg.m_status.m_ppuMode = +PPUMode::VBLANK;
                } else {
                    m_reg.m_status.m_ppuMode = +PPUMode::HBLANK;
                }
            }
            break;
        case +PPUMode::VBLANK:
            if (m_currentLineDotTickCount == 456) {
                ++m_reg.m_ly;
                m_currentLineDotTickCount = 0;
                if (m_reg.m_ly == 154) {
                    m_reg.m_ly = 0;
                    m_reg.m_status.m_ppuMode = +PPUMode::OAM_SCAN;
                }
            }
            break;
    }
}
} // namespace ez