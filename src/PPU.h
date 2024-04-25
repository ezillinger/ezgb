#pragma once
#include "Base.h"
#include "IO.h"

namespace ez {

enum class StatIRQSources { MODE_0 = 0, MODE_1, MODE_2, LY, NUM_SOURCES };

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

    void setStatIRQHigh(StatIRQSources src);
    void updateLyLyc();

    int m_currentLineDotTickCount = 0;

    std::array<bool, +StatIRQSources::NUM_SOURCES> m_statIRQSources{};
    bool m_statIRQ = false;
    bool m_statIRQRisingEdge = false;

    IO& m_io;
    IORegisters& m_reg;
    std::vector<uint8_t> m_vram = std::vector<uint8_t>(VRAM_BYTES, 0u);
    std::vector<uint8_t> m_display = std::vector<uint8_t>(size_t(DISPLAY_WIDTH * DISPLAY_HEIGHT), 0u);
};
} // namespace ez