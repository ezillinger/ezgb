#pragma once

#include "AppState.h"
#include "Base.h"
#include <cmath>

namespace ez {

enum class RunResult { CONTINUE, DRAW };

class Runner {
  public:
    Runner(AppState& state): m_state(state) {};
    RunResult tick();

  private:
    void tickEmuOnce(); 

    int m_ticksSinceLastDraw = 0;
    static constexpr auto TICKS_PER_DRAW =
        int((std::round(16.6666ms / (4 * Emulator::MASTER_CLOCK_PERIOD))));
    AppState& m_state;
};
} // namespace ez