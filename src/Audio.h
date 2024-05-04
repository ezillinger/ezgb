#pragma once
#include "Base.h"
#include "ThirdParty_SDL.h"

namespace ez::audio {
    using Sample = std::array<float, 2>;

    static constexpr int SAMPLE_RATE = 44'100;
    static constexpr int FORMAT = AUDIO_F32;
    static constexpr int BUFFER_SIZE = 2048;
    static constexpr int NUM_CHANNELS = 2;

    using SinkFunc = std::function<void(std::span<const Sample>)>;
}