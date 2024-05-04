#pragma once
#include "Audio.h"
#include "Base.h"
#include "IO.h"
#include "ThirdParty_SDL.h"

namespace ez {

class PulseOsc {
  public:
    struct State {
        bool m_enabled = false;
        uint8_t m_duty = 0;

        uint8_t m_envelopeInitial = 0;
        bool m_envelopeIncreasing = false;
        uint8_t m_envelopePace = 0;

        uint8_t m_sweepPace = 0;
        bool m_sweepIncreasing = false;
        uint8_t m_sweepStep = 0;

        uint16_t m_period = 0; // freq period

        int8_t m_lengthInitial = 0;
        bool m_lengthEnable = false;
    };

    static constexpr int MAX_OUTPUT = 0xF;

    PulseOsc(bool hasSweep)
        : m_hasSweep(hasSweep){};
    
    void update(const State& state) { m_state = state; }
    void trigger();
    void tick();

    uint8_t get_sample() const;

  private:
    int get_initial_freq_counter() const;
    State m_state;

    int m_64HzCounter = 0; // envelope
    int m_128HzCounter = 0; // sweep
    int m_256HzCounter = 0; // length

    int m_currentVolume = 0;
    int m_envelopeCounter = 0;
    int m_lengthCounter = 0;
    int m_freqCounter = 0;

    uint8_t m_dutyCyleCounter = 0;

    int m_sweepCounter = 0;

    const bool m_hasSweep = false;
};

class NoiseOsc {
  public:
    struct State {
        bool m_enabled = false;

        uint8_t m_envelopeInitial = 0;
        bool m_envelopeIncreasing = false;
        uint8_t m_envelopePace = 0;

        int8_t m_lengthInitial = 0;
        bool m_lengthEnable = false;

        int m_clockShift = 0;
        int m_clockDivider = 0;
        bool m_lfsrWidthIs7Bit = false; // otherwise 15
    };

    static constexpr int MAX_OUTPUT = 0xF;

    NoiseOsc() = default;

    void update(const State& state) { m_state = state; }
    void trigger();
    void tick();

    uint8_t get_sample() const;

  private:
    int get_initial_freq_counter() const;
    State m_state;

    int m_64HzCounter = 0; // envelope
    int m_256HzCounter = 0; // length

    int m_currentVolume = 0;
    int m_envelopeCounter = 0;
    int m_lengthCounter = 0;

    int m_freqCounter = 0;
    uint16_t m_lfsr = 0xFFFF;
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
    void update_osc1();
    void update_osc2();
    void update_osc4();

    IORegisters& m_reg;
    chrono::nanoseconds m_timeSinceEmitSample = 0ns;
    std::vector<audio::Sample> m_outputBuffer;

    PulseOsc m_osc1{true}; // has sweep
    PulseOsc m_osc2{false}; // no sweep
    NoiseOsc m_osc4{};
};

} // namespace ez