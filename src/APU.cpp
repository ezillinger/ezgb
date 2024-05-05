
#include "APU.h"

namespace ez {

static constexpr int T_CYCLES_PER_512HZ_PERIOD = 8192;
static constexpr int T_CYCLES_PER_256HZ_PERIOD = T_CYCLES_PER_512HZ_PERIOD * 2;
static constexpr int T_CYCLES_PER_128HZ_PERIOD = T_CYCLES_PER_256HZ_PERIOD * 2;
static constexpr int T_CYCLES_PER_64HZ_PERIOD = T_CYCLES_PER_128HZ_PERIOD * 2;

static constexpr int OSC_MAX_DIGITAL_OUTPUT = 0xF;

static void update_pulse_osc(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3,
                             uint8_t byte4, PulseOsc& osc) {

    PulseOsc::State state{};

    state.m_sweepPace = (byte0 & 0b0111'0000) >> 4;
    state.m_sweepIncreasing = !bool(byte0 & 0b1000);
    state.m_sweepStep = byte0 & 0b111;

    const uint8_t dutyBits = (0b1100'0000 & byte1) >> 6;
    state.m_duty = dutyBits;

    state.m_lengthInitial = 0b0011'1111 & byte1;

    state.m_envelopeInitial = (0b1111'0000 & byte2) >> 4;
    state.m_envelopeIncreasing = 0x0000'1000 & byte2;
    state.m_envelopePace = byte2 & 0b111;

    state.m_period = byte3 | ((byte4 & 0b111) << 8);
    state.m_lengthEnable = byte4 & 0b0100'0000;

    osc.update(state);
}

APU::APU(IORegisters& io)
    : m_reg(io){};

APU::~APU() {}

uint8_t APU::read_addr(uint16_t addr) const {

    // todo, delete me
    EZ_MSVC_WARN_DISABLE(4065)
    EZ_ENSURE(AUDIO_ADDR_RANGE.containsExclusive(addr));
    switch (addr) {
        default: {
            const auto offset = addr - IO_ADDR_RANGE.m_min;
            return reinterpret_cast<uint8_t*>(&m_reg)[offset];
        } break;
    }
}

void APU::write_addr(uint16_t addr, uint8_t val) {

    EZ_ENSURE(AUDIO_ADDR_RANGE.containsExclusive(addr));
    switch (addr) {
        // todo, this is really inaccurate, parameters are updated on different cycles, not just on
        // trigger
        case +IOAddr::NR14: {
            m_reg.m_nr14 = val;
            if (0b1000'0000 & val) {
                update_osc1();
                m_osc1.trigger();
            }
            break;
        }
        case +IOAddr::NR24: {
            m_reg.m_nr24 = val;
            if (0b1000'0000 & val) {
                update_osc2();
                m_osc2.trigger();
            }
            break;
        }
        case +IOAddr::NR34: {
            m_reg.m_nr34 = val;
            if (0b1000'0000 & val) {
                update_osc3();
                m_osc3.trigger();
            }
            break;
        }
        case +IOAddr::NR44: {
            m_reg.m_nr44 = val;
            if (0b1000'0000 & val) {
                update_osc4();
                m_osc4.trigger();
            }
            break;
        }
        default: {
            const auto offset = addr - IO_ADDR_RANGE.m_min;
            reinterpret_cast<uint8_t*>(&m_reg)[offset] = val;
        } break;
    }
}

void APU::update_osc1() {

    update_pulse_osc(m_reg.m_nr10, m_reg.m_nr11, m_reg.m_nr12, m_reg.m_nr13, m_reg.m_nr14, m_osc1);
}

void APU::update_osc2() {

    update_pulse_osc(uint8_t(0), // not used - no pulse
                     m_reg.m_nr21,
                     m_reg.m_nr22,
                     m_reg.m_nr23,
                     m_reg.m_nr24,
                     m_osc2);
}

void APU::update_osc3() {

    WaveOsc::State state{};

    state.m_lengthInitial = m_reg.m_nr31;

    state.m_period = m_reg.m_nr33 | ((m_reg.m_nr34 & 0b111) << 8);
    state.m_lengthEnable = m_reg.m_nr34 & 0b0100'0000;

    state.m_volume = (m_reg.m_nr32 & 0b0110'0000) >> 5;

    m_osc3.update(state);
}

void APU::update_osc4() {
    NoiseOsc::State state{};

    state.m_lengthInitial = 0b0011'1111 & m_reg.m_nr41;

    state.m_envelopeInitial = (0b1111'0000 & m_reg.m_nr42) >> 4;
    state.m_envelopeIncreasing = 0x0000'1000 & m_reg.m_nr42;
    state.m_envelopePace = m_reg.m_nr42 & 0b111;

    state.m_clockDivider = 0b111 & m_reg.m_nr43;
    state.m_lfsrWidthIs7Bit = 0b1000 & m_reg.m_nr43;
    state.m_clockShift = (0b1111'0000 & m_reg.m_nr43) >> 4;

    state.m_lengthEnable = m_reg.m_nr44 & 0b0100'0000;

    m_osc4.update(state);
}

void ez::APU::tick() {
    m_timeSinceEmitSample += MASTER_CLOCK_PERIOD;
    static constexpr auto samplePeriod = 22'675ns; // 44.1khz

    m_osc1.tick();
    m_osc2.tick();
    m_osc3.tick();
    m_osc4.tick();

    while (m_timeSinceEmitSample > samplePeriod) {
        m_timeSinceEmitSample -= samplePeriod;

        const auto b1 = m_osc1.get_sample();
        const auto b2 = m_osc2.get_sample();
        const auto b3 = m_osc3.get_sample(m_reg.m_wavePattern);
        const auto b4 = m_osc4.get_sample();

        int numChannels = 4;

        float sample = (float(b1) / OSC_MAX_DIGITAL_OUTPUT + float(b2) / OSC_MAX_DIGITAL_OUTPUT +
                        +float(b3) / OSC_MAX_DIGITAL_OUTPUT + float(b4) / OSC_MAX_DIGITAL_OUTPUT) /
                       numChannels;
        m_outputBuffer.push_back(audio::Sample{sample, sample});
    }
}

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
                        m_state.m_period = newPeriod;
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
        m_lfsr = (m_lfsr >> 1) | (xorResult << 14);

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

    return val >> shiftAmt;
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

} // namespace ez