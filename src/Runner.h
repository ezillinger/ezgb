#pragma once

#include "AppState.h"
#include "Audio.h"
#include "Base.h"
#include <cmath>

namespace ez {

enum class RunResult {
    CONTINUE,
    DRAW
};

class Runner {
  public:
    Runner(AppState& state) : m_state(state){};
    RunResult tick(const InputState& input, audio::SinkFunc putSamples);

  private:
    void tick_emu_once(const InputState& input, audio::SinkFunc putSamples);

    int m_ticksSinceLastDraw = 0;
    static constexpr auto TICKS_PER_DRAW = 70'224; // dots per v-sync;
    AppState& m_state;
};
} // namespace ez