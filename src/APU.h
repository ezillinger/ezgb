#pragma once
#include "Audio.h"
#include "Base.h"
#include "IO.h"
#include "ThirdParty_SDL.h"

namespace ez {

struct PulseState {
    bool m_enabled = false;
    uint8_t m_duty = 0;

    uint8_t m_envelopeInitial = 0;
    bool m_envelopeIncreasing = false;
    uint8_t m_envelopePace = 0;

    uint16_t m_period = 0;

    int8_t m_lengthInitial = 0;
    bool m_lengthEnable = false;
};

static constexpr int T_CYCLES_PER_512HZ_PERIOD = 8192;
static constexpr int T_CYCLES_PER_256HZ_PERIOD = T_CYCLES_PER_512HZ_PERIOD * 2;
static constexpr int T_CYCLES_PER_64HZ_PERIOD = T_CYCLES_PER_512HZ_PERIOD * 4;

class PulseOsc {
  public:
    PulseOsc() = default;

    void update(const PulseState& state) { m_state = state; }
    void trigger() {
        m_state.m_enabled = true;
        m_currentVolume = m_state.m_envelopeInitial;
        m_64HzCounter = 0;
        m_256HzCounter = 0;
        m_lengthCounter = m_state.m_lengthInitial;
    }
    void tick();

    uint8_t get_sample() const;

  private:
    int get_max_freq_count() const;
    PulseState m_state;

    int m_64HzCounter = 0;
    int m_256HzCounter = 0;
    int m_512HzCounter = 0;

    int m_currentVolume = 0;
    int m_envelopeCounter = 0;
    int m_lengthCounter = 0;
    int m_freqCounter = 0;
};

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
    void updateOsc1();
    void updateOsc2();
    static void updatePulse(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, PulseOsc& osc);

    void tickOsc1();
    IORegisters& m_reg;
    chrono::nanoseconds m_timeSinceEmitSample = 0ns;
    std::vector<audio::Sample> m_outputBuffer;

    PulseOsc m_osc1{};
    PulseOsc m_osc2{};
};

} // namespace ez