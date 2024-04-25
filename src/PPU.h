#pragma once
#include "Base.h"
#include "IO.h"

namespace ez {



class PPU {

  public:
    static constexpr size_t VRAM_BYTES = 8 * 1024;
    static constexpr uint16_t VRAM_BASE_ADDR = 0x8000;
    static constexpr int DISPLAY_WIDTH = 160;
    static constexpr int DISPLAY_HEIGHT = 144;

    PPU(IO& io);

    void tick();

    uint8_t readAddr(uint16_t) const;
    uint16_t readAddr16(uint16_t) const;

    void writeAddr(uint16_t, uint8_t);
    void writeAddr16(uint16_t, uint16_t);

    void dumpDisplay() const;

  private:

    int m_currentLineDotTickCount = 0;

    IO& m_io;
    LCDRegisters& m_reg;
    std::vector<uint8_t> m_vram = std::vector<uint8_t>(VRAM_BYTES, 0u);
    std::vector<uint8_t> m_display = std::vector<uint8_t>(size_t(DISPLAY_WIDTH * DISPLAY_HEIGHT), 0u);
};
} // namespace ez