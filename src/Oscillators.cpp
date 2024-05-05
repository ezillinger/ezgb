#include "Oscillators.h"

namespace ez {

static constexpr int T_CYCLES_PER_512HZ_PERIOD = 8192;
static constexpr int T_CYCLES_PER_256HZ_PERIOD = T_CYCLES_PER_512HZ_PERIOD * 2;
static constexpr int T_CYCLES_PER_128HZ_PERIOD = T_CYCLES_PER_256HZ_PERIOD * 2;
static constexpr int T_CYCLES_PER_64HZ_PERIOD = T_CYCLES_PER_128HZ_PERIOD * 2;

void PulseOsc::trigger() {
    m_state.m_enabled = true;
    m_currentVolume = m_state.m_envelopeInitial;
    m_lengthCounter = m_state.m_lengthInitial;
    m_envelopeCounter = 0;
    m_dutyCyleCounter = 0;
    m_freqCounter = get_initial_freq_counter();
}

uint8_t PulseOsc::get_sample() const {

    if (!m_state.m_enabled) {
        return 0;
    }

    const auto duty = m_state.m_duty == 0b00   ? 1
                      : m_state.m_duty == 0b01 ? 2
                      : m_state.m_duty == 0b10 ? 4
                                               : 6;
    ez_assert(m_state.m_duty < 8);
    return uint8_t(m_currentVolume * (m_dutyCyleCounter < duty ? 1 : 0));
}

int PulseOsc::get_initial_freq_counter() const { return (2048 - m_state.m_period) * 4; }

void PulseOsc::tick() {

    // todo, this should all be tied to DIV, not running on its own clock

    // envelope
    ++m_64HzCounter;
    if (m_64HzCounter == T_CYCLES_PER_64HZ_PERIOD) {
        m_64HzCounter = 0;
        if (m_state.m_envelopePace != 0) {
            ++m_envelopeCounter;
            if (m_envelopeCounter >= m_state.m_envelopePace) {
                m_envelopeCounter = 0;
                m_currentVolume += m_state.m_envelopeIncreasing ? 1 : -1;
                m_currentVolume = clamp(m_currentVolume, 0, OSC_MAX_DIGITAL_OUTPUT);
            }
        }
    }

    if (m_hasSweep) {
        ++m_128HzCounter;
        if (m_128HzCounter >= T_CYCLES_PER_128HZ_PERIOD) {
            m_128HzCounter = 0;
            if (m_state.m_sweepPace != 0) {
                ++m_sweepCounter;
                if (m_sweepCounter >= m_state.m_sweepPace) {
                    m_sweepCounter = 0;
                    const auto sweepIncrement = m_state.m_period >> m_state.m_sweepStep;
                    const auto newPeriod =
                        m_state.m_period +
                        (m_state.m_sweepIncreasing ? sweepIncrement : -sweepIncrement);
                    if (!iRange(0, 2048).containsExclusive(newPeriod)) {
                        m_state.m_enabled = false;
                    } else {
                        m_state.m_period = uint16_t(newPeriod);
                    }
                }
            }
        }
    }

    ++m_256HzCounter;
    if (m_256HzCounter >= T_CYCLES_PER_256HZ_PERIOD) {
        if (m_state.m_lengthEnable) {
            m_256HzCounter = 0;
            ++m_lengthCounter;
            if (m_lengthCounter >= 64) {
                m_state.m_enabled = false;
            }
        }
    }

    --m_freqCounter;
    if (m_freqCounter <= 0) {
        m_freqCounter = get_initial_freq_counter();
        m_dutyCyleCounter = (m_dutyCyleCounter + 1) % 8;
    }
}

void NoiseOsc::trigger() {
    m_state.m_enabled = true;
    m_currentVolume = m_state.m_envelopeInitial;
    m_lengthCounter = m_state.m_lengthInitial;
    m_envelopeCounter = 0;
    m_lfsr = 0xFFFF;
    m_freqCounter = get_initial_freq_counter();
}

void NoiseOsc::tick() {
    // todo, this should all be tied to DIV, not running on its own clock

    // envelope
    ++m_64HzCounter;
    if (m_64HzCounter == T_CYCLES_PER_64HZ_PERIOD) {
        m_64HzCounter = 0;
        if (m_state.m_envelopePace != 0) {
            ++m_envelopeCounter;
            if (m_envelopeCounter >= m_state.m_envelopePace) {
                m_envelopeCounter = 0;
                m_currentVolume += m_state.m_envelopeIncreasing ? 1 : -1;
                m_currentVolume = clamp(m_currentVolume, 0, OSC_MAX_DIGITAL_OUTPUT);
            }
        }
    }

    ++m_256HzCounter;
    if (m_256HzCounter >= T_CYCLES_PER_256HZ_PERIOD) {
        if (m_state.m_lengthEnable) {
            m_256HzCounter = 0;
            ++m_lengthCounter;
            if (m_lengthCounter >= 64) {
                m_state.m_enabled = false;
            }
        }
    }

    --m_freqCounter;
    if (m_freqCounter <= 0) {
        m_freqCounter = get_initial_freq_counter();

        // wtf? https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
        const auto xorResult = (m_lfsr & 0b01) ^ ((m_lfsr & 0b10) >> 1);
        m_lfsr = uint16_t((m_lfsr >> 1) | (xorResult << 14));

        if (m_state.m_lfsrWidthIs7Bit) {
            m_lfsr &= ~(0b1 << 6);
            m_lfsr |= xorResult << 6;
        }
    }
}

uint8_t NoiseOsc::get_sample() const {
    if (!m_state.m_enabled) {
        return 0;
    }
    return uint8_t(m_currentVolume * (~m_lfsr & 0b1));
}

int NoiseOsc::get_initial_freq_counter() const {
    ez_assert(m_state.m_clockDivider < 8);
    return (m_state.m_clockDivider > 0 ? (m_state.m_clockDivider << 4) : 8) << m_state.m_clockShift;
}

void WaveOsc::trigger() {
    m_state.m_enabled = true;
    m_lengthCounter = m_state.m_lengthInitial;
    m_freqCounter = get_initial_freq_counter();
    m_waveCounter = 0;
    m_oddTick = false;
}

uint8_t WaveOsc::get_sample(std::span<const uint8_t, 16> waveData) const {

    if (!m_state.m_enabled) {
        return 0;
    }

    ez_assert(m_waveCounter < 32);

    const bool high = m_waveCounter % 2;
    const auto byte = waveData[m_waveCounter / 2];

    const auto val = high ? (0xF0 & byte) >> 4 : 0x0F & byte;
    const auto shiftAmt = m_state.m_volume == 0   ? 4
                          : m_state.m_volume == 1 ? 0
                          : m_state.m_volume == 2 ? 1
                          : m_state.m_volume == 3 ? 2
                                                  : 3;

    return uint8_t(val >> shiftAmt);
}

int WaveOsc::get_initial_freq_counter() const { return (2048 - m_state.m_period) * 4; }

void WaveOsc::tick() {

    // todo, this should all be tied to DIV, not running on its own clock

    // envelope
    ++m_256HzCounter;
    if (m_256HzCounter >= T_CYCLES_PER_256HZ_PERIOD) {
        if (m_state.m_lengthEnable) {
            m_256HzCounter = 0;
            ++m_lengthCounter;
            static constexpr auto lengthOverflow = 256;
            if (m_lengthCounter >= lengthOverflow) {
                m_state.m_enabled = false;
            }
        }
    }
    // half the frequency compared to other oscs
    m_oddTick = !m_oddTick;
    if (!m_oddTick || true) {
        --m_freqCounter;
        if (m_freqCounter <= 0) {
            m_freqCounter = get_initial_freq_counter();
            m_waveCounter = (m_waveCounter + 1) % 32;
        }
    }
}

}