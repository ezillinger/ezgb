#pragma once
#include "Base.h"
#include "ThirdParty_SDL.h"

namespace ez {

namespace audio {
using Sample = std::array<float, 2>;

static constexpr int SAMPLE_RATE = 44'100;
static constexpr int FORMAT = AUDIO_F32;
static constexpr int BUFFER_SIZE = 2048;
static constexpr int NUM_CHANNELS = 2;

using SinkFunc = std::function<void(std::span<const Sample>)>;
} // namespace audio

// approximates a windowed average filter without storing a full window
class LowPassFilter {
  public:
    LowPassFilter(int capacity)
        : m_capacity(capacity) {
        ez_assert(m_capacity > 1);
    }

    float process(float v) {
        if (m_currentSize == m_capacity) {
            m_average -= m_average / m_currentSize;
        } else {
            ++m_currentSize;
        }
        m_average += v / m_capacity;

        return m_average;
    }

  protected:
    int m_capacity = 0;
    int m_currentSize = 0;
    float m_average = 0.0f;
};

} // namespace ez