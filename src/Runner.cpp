#include "Runner.h"

namespace ez {

RunResult ez::Runner::tick() {
    auto ret = RunResult::CONTINUE;
    if (m_state.m_isPaused) {
        if (m_state.m_singleStep) {
            tickEmuOnce();
            m_state.m_singleStep = false;
        }
        ret = RunResult::DRAW;
    } else {
        tickEmuOnce();
        ++m_ticksSinceLastDraw;
        if (m_ticksSinceLastDraw == TICKS_PER_DRAW) {
            ret = RunResult::DRAW;
        }
    }
    if(ret == RunResult::DRAW){
        m_ticksSinceLastDraw = 0;
    }
    return ret;
}

void Runner::tickEmuOnce() {
    bool shouldBreak = false;
    m_state.m_emu->tick();
    shouldBreak = m_state.m_emu->getPC() == m_state.m_debugSettings.m_breakOnPC;
    const auto currentOp = m_state.m_emu->getCurrentOpCode();
    shouldBreak |= currentOp.m_addr == (currentOp.m_prefixed
                       ? m_state.m_debugSettings.m_breakOnOpCodePrefixed
                       : m_state.m_debugSettings.m_breakOnOpCode);


    if(shouldBreak){
        log_info("Debug Break!");
        m_state.m_isPaused = true;
    }
}

} // namespace ez