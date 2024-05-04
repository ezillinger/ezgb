
#include "APU.h"

namespace ez {

APU::APU(IORegisters& io) : m_reg(io){};

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
        case +IOAddr::NR14: {
            m_reg.m_nr14 = val;
            if(0b1000'0000 & val){
                updateOsc1();
                m_osc1.trigger();
            }
            break;
        }
        case +IOAddr::NR24: {
            m_reg.m_nr24 = val;
            if(0b1000'0000 & val){
                updateOsc2();
                m_osc2.trigger();
            }
            break;
        }
        default: {
            const auto offset = addr - IO_ADDR_RANGE.m_min;
            reinterpret_cast<uint8_t*>(&m_reg)[offset] = val;
        } break;
    }
}


void APU::updateOsc1() {

    auto byte1 = m_reg.m_nr11;
    auto byte2 = m_reg.m_nr12;
    auto byte3 = m_reg.m_nr13;
    auto byte4 = m_reg.m_nr14;

    updatePulse(byte1, byte2, byte3, byte4, m_osc1);
    //todo, sweep
}

void APU::updateOsc2() {

    auto byte1 = m_reg.m_nr21;
    auto byte2 = m_reg.m_nr22;
    auto byte3 = m_reg.m_nr23;
    auto byte4 = m_reg.m_nr24;

    updatePulse(byte1, byte2, byte3, byte4, m_osc2);
    //todo, sweep
}

void APU::updatePulse(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, PulseOsc& osc) {

    PulseState state{};
    const uint8_t dutyBits = (0b1100'0000 & byte1) >> 6;
    state.m_duty = dutyBits;
    state.m_envelopeInitial = (0b1111'0000 & byte2) >> 4;
    state.m_envelopeIncreasing = 0x0000'1000 & byte2;
    state.m_envelopePace = byte2 & 0b111;

    state.m_period = byte3 | ((byte4 & 0b111) << 8);
    state.m_lengthEnable = byte4 & 0b0100'0000;

    osc.update(state);
}

uint8_t PulseOsc::get_sample() const {
    const auto duty = m_state.m_duty == 0b00   ? 0.125f
                      : m_state.m_duty == 0b01 ? 0.25f
                      : m_state.m_duty == 0b10 ? 0.5f
                                               : 0.75f;
    const auto currentCycleT = m_freqCounter / float(get_max_freq_count());
    return uint8_t(m_currentVolume * (currentCycleT > duty ? 1 : 0));
}

int PulseOsc::get_max_freq_count() const { 
    return (2048 - m_state.m_period);
}

void PulseOsc::tick() {

    static constexpr auto maxVolume = 0x0F;

    // envelope
    ++m_64HzCounter;
    if (m_64HzCounter == T_CYCLES_PER_64HZ_PERIOD) {
        m_64HzCounter = 0;
        if (m_state.m_envelopePace != 0) {
            ++m_envelopeCounter;
            if (m_state.m_envelopePace == m_envelopeCounter) {
                m_envelopeCounter = 0;
                m_currentVolume += m_state.m_envelopeIncreasing ? 1 : -1;
                m_currentVolume = clamp(m_currentVolume, 0, maxVolume);
            }
        }
    }

    // todo, this should be tied to DIV
    ++m_256HzCounter;
    if (m_256HzCounter >= T_CYCLES_PER_256HZ_PERIOD) {
        if (m_state.m_lengthEnable) {
            m_256HzCounter = 0;
            ++m_lengthCounter;
            if (m_lengthCounter == 64) {
                m_state.m_enabled = false;
            }
        }
    }

    ++m_512HzCounter;
    if (m_512HzCounter >= T_CYCLES_PER_512HZ_PERIOD) {
        m_512HzCounter = 0;
        ++m_freqCounter;
        if (m_freqCounter >= get_max_freq_count()) {
            m_freqCounter = 0;
        }
    }
}

void ez::APU::tick() {
    m_timeSinceEmitSample += MASTER_CLOCK_PERIOD;
    static constexpr auto samplePeriod = 22'675ns; // 44.1khz

    m_osc1.tick();
    m_osc2.tick();

    while (m_timeSinceEmitSample > samplePeriod) {
        m_timeSinceEmitSample -= samplePeriod;

        auto b1 = m_osc1.get_sample();
        auto b2 = m_osc2.get_sample();

        int numChannels = 4;

        float sample = (b1 / 15.0f) + (b2 / 15.0f) / numChannels;
        m_outputBuffer.push_back(audio::Sample{sample, sample});
    }
}

} // namespace ez