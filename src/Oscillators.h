#pragma once
#include "Base.h"

namespace ez {

static constexpr int OSC_MAX_DIGITAL_OUTPUT = 0xF;

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

        uint8_t m_lengthInitial = 0;
        bool m_lengthEnable = false;
    };

    PulseOsc(bool hasSweep)
        : m_hasSweep(hasSweep){};

    void update(const State& state) { m_state = state; }
    void trigger();
    void tick();
    bool enabled() const { return m_state.m_enabled; }

        uint8_t get_sample() const;

      private:
        int get_initial_freq_counter() const;
        State m_state;

        int m_64HzCounter = 0;  // envelope
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

        uint8_t m_lengthInitial = 0;
        bool m_lengthEnable = false;

        uint8_t m_envelopeInitial = 0;
        bool m_envelopeIncreasing = false;
        uint8_t m_envelopePace = 0;

        uint8_t m_clockDivider = 0;
        uint8_t m_clockShift = 0;
        bool m_lfsrWidthIs7Bit = false;
    };

    NoiseOsc() = default;

    void update(const State& state) { m_state = state; }
    void trigger();
    void tick();
    bool enabled() const { return m_state.m_enabled; }

    uint8_t get_sample() const;

  private:
    int get_initial_freq_counter() const;
    State m_state;

    int m_64HzCounter = 0;  // envelope
    int m_256HzCounter = 0; // length

    int m_currentVolume = 0;
    int m_envelopeCounter = 0;
    int m_lengthCounter = 0;

    int m_freqCounter = 0;
    uint16_t m_lfsr = 0xFFFF;
};

class WaveOsc {
  public:
    struct State {
        bool m_enabled = false;

        uint8_t m_volume = 0;

        uint8_t m_lengthInitial = 0;
        bool m_lengthEnable = false;

        uint16_t m_period = 0;
    };

    WaveOsc() = default;

    void update(const State& state) { m_state = state; }
    void trigger();
    void tick();
    bool enabled() const { return m_state.m_enabled; }

    uint8_t get_sample(std::span<const uint8_t, 16> waveData) const;

  private:
    int get_initial_freq_counter() const;
    State m_state;

    int m_256HzCounter = 0; // length
    int m_lengthCounter = 0;

    bool m_oddTick = false;
    int m_freqCounter = 0;
    int m_waveCounter = 0;
};

} // namespace ez