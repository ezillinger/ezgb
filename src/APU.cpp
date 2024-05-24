#include "APU.h"

namespace ez {

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

APU::APU(Addressable<IORegisters>& io)
    : m_reg(io){};

APU::~APU() {}

uint8_t APU::read_addr(uint16_t addr) const {

    EZ_ENSURE(AUDIO_ADDR_RANGE.containsExclusive(addr));
    switch (addr) {
        case +IOAddr::NR52: {
            auto v = m_reg->m_nr52;
            v |= m_osc1.enabled() ? 0b0001 : 0;
            v |= m_osc2.enabled() ? 0b0010 : 0;
            v |= m_osc3.enabled() ? 0b0100 : 0;
            v |= m_osc4.enabled() ? 0b1000 : 0;
            return v;
        }
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
        case +IOAddr::NR52: {
            const bool apuOn = 0b1000'0000 & val;
            if(!apuOn){
                log_info("APU turned off!");
            }
            m_reg->m_nr52 = val;
            break;
        }
        case +IOAddr::NR14: {
            m_reg->m_nr14 = val;
            if (0b1000'0000 & val) {
                update_osc1();
                m_osc1.trigger();
            }
            break;
        }
        case +IOAddr::NR24: {
            m_reg->m_nr24 = val;
            if (0b1000'0000 & val) {
                update_osc2();
                m_osc2.trigger();
            }
            break;
        }
        case +IOAddr::NR30: {
            m_reg->m_nr30 = val;
            update_osc3();
            break;
        }
        case +IOAddr::NR34: {
            m_reg->m_nr34 = val;
            if (0b1000'0000 & val) {
                update_osc3();
                m_osc3.trigger();
            }
            break;
        }
        case +IOAddr::NR44: {
            m_reg->m_nr44 = val;
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
    update_pulse_osc(m_reg->m_nr10, m_reg->m_nr11, m_reg->m_nr12, m_reg->m_nr13, m_reg->m_nr14, m_osc1);
}

void APU::update_osc2() {

    update_pulse_osc(uint8_t(0), // not used - no pulse
                     m_reg->m_nr21,
                     m_reg->m_nr22,
                     m_reg->m_nr23,
                     m_reg->m_nr24,
                     m_osc2);
}

void APU::update_osc3() {

    WaveOsc::State state{};

    state.m_enabled = m_reg->m_nr30 & 0b1000'0000;
    state.m_lengthInitial = m_reg->m_nr31;

    state.m_period = m_reg->m_nr33 | ((m_reg->m_nr34 & 0b111) << 8);
    state.m_lengthEnable = m_reg->m_nr34 & 0b0100'0000;

    state.m_volume = (m_reg->m_nr32 & 0b0110'0000) >> 5;

    m_osc3.update(state);
}

void APU::update_osc4() {
    NoiseOsc::State state{};

    state.m_lengthInitial = 0b0011'1111 & m_reg->m_nr41;

    state.m_envelopeInitial = (0b1111'0000 & m_reg->m_nr42) >> 4;
    state.m_envelopeIncreasing = 0x0000'1000 & m_reg->m_nr42;
    state.m_envelopePace = m_reg->m_nr42 & 0b111;

    state.m_clockDivider = 0b111 & m_reg->m_nr43;
    state.m_lfsrWidthIs7Bit = 0b1000 & m_reg->m_nr43;
    state.m_clockShift = (0b1111'0000 & m_reg->m_nr43) >> 4;

    state.m_lengthEnable = m_reg->m_nr44 & 0b0100'0000;

    m_osc4.update(state);
}

void ez::APU::tick() {

    m_timeSinceEmitSample += MASTER_CLOCK_PERIOD;
    static constexpr auto samplePeriod = 22'675ns; // 44.1khz

    const bool apuEnabled = m_reg->m_nr52 & 0b1000'0000;
    auto lrSample = audio::Sample{0.0f, 0.0f};
    if (apuEnabled) {
        m_osc1.tick();
        m_osc2.tick();
        m_osc3.tick();
        m_osc4.tick();

        const auto b1 = float(m_osc1.get_sample()) / OSC_MAX_DIGITAL_OUTPUT;
        const auto b2 = float(m_osc2.get_sample()) / OSC_MAX_DIGITAL_OUTPUT;
        const auto b3 = float(m_osc3.get_sample(m_reg->m_wavePattern)) / OSC_MAX_DIGITAL_OUTPUT;
        const auto b4 = float(m_osc4.get_sample()) / OSC_MAX_DIGITAL_OUTPUT;

        static constexpr int numChannels = 4;
        const auto panMap = m_reg->m_nr51;

        float leftSample =
            ((0b0001'0000 & panMap) ? b1 : 0.0f) + ((0b0010'0000 & panMap) ? b2 : 0.0f) +
            ((0b0100'0000 & panMap) ? b3 : 0.0f) + ((0b1000'0000 & panMap) ? b4 : 0.0f);

        float rightSample =
            ((0b0000'0001 & panMap) ? b1 : 0.0f) + ((0b0000'0010 & panMap) ? b2 : 0.0f) +
            ((0b0000'0100 & panMap) ? b3 : 0.0f) + ((0b0000'1000 & panMap) ? b4 : 0.0f);

        leftSample /= numChannels;
        rightSample /= numChannels;

        // weird quirk: volume of 0 == 1 and volume of 7 == 8
        const auto lVolume = clamp((m_reg->m_nr50 & 0b0111'0000) >> 4, 1, 7);
        const auto rVolume = clamp(m_reg->m_nr50 & 0b0000'0111, 1, 7);

        const auto lRatio = lerpInverse(lVolume, 0, 7);
        const auto rRatio = lerpInverse(rVolume, 0, 7);

        leftSample *= lRatio;
        rightSample *= rRatio;

        // we're pumping perfect square waves into the DAC, do some rudimentary filtering
        leftSample = m_filterL.process(leftSample);
        rightSample = m_filterR.process(rightSample);

        lrSample = {leftSample, rightSample};
    }

    while (m_timeSinceEmitSample > samplePeriod) {
        m_timeSinceEmitSample -= samplePeriod;

        m_outputBuffer.push_back(lrSample);
    }
}

} // namespace ez