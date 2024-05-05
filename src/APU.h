#pragma once
#include "Audio.h"
#include "Base.h"
#include "IO.h"
#include "Oscillators.h"

namespace ez {

class APU {

  public:
    friend class Tester;

    static constexpr auto AUDIO_ADDR_RANGE = iRange{0xFF10, 0xFF40};
    static constexpr int CHANNELS = 4;

    APU(IORegisters& io);
    ~APU();

    uint8_t read_addr(uint16_t addr) const;
    void write_addr(uint16_t addr, uint8_t val);

    void tick();

    std::span<const audio::Sample> get_samples() const { return {m_outputBuffer}; }
    void clear_buffer() { m_outputBuffer.clear(); }

  protected:
    void update_osc1();
    void update_osc2();
    void update_osc3();
    void update_osc4();

    IORegisters& m_reg;
    chrono::nanoseconds m_timeSinceEmitSample = 0ns;
    std::vector<audio::Sample> m_outputBuffer;

    LowPassFilter m_filterL{5}; // arbitrary filter size
    LowPassFilter m_filterR{5};

    PulseOsc m_osc1{true}; // has sweep
    PulseOsc m_osc2{false}; // no sweep
    WaveOsc m_osc3{};
    NoiseOsc m_osc4{};
};

} // namespace ez