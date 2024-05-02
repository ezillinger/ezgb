#include "Runner.h"

namespace ez {

RunResult ez::Runner::tick(const JoypadState& input) {
    auto ret = RunResult::CONTINUE;
    if (m_state.m_isPaused) {
        if (m_state.m_stepToNextInstr) {
            while (true) {
                tickEmuOnce(input);
                if (m_state.m_emu->getCyclesUntilNextInstruction() == 0) {
                    break;
                }
            }
            m_state.m_stepToNextInstr = false;
        }
        else if(m_state.m_stepOneCycle){
            tickEmuOnce(input);
            m_state.m_stepOneCycle = false;
        }
        ret = RunResult::DRAW;
    } else {
        tickEmuOnce(input);
        ++m_ticksSinceLastDraw;
        if (m_ticksSinceLastDraw == TICKS_PER_DRAW) {
            ret = RunResult::DRAW;
        }
    }
    if (ret == RunResult::DRAW) {
        m_ticksSinceLastDraw = 0;
    }
    return ret;
}

void Runner::tickEmuOnce(const JoypadState& input) {
    bool shouldBreak = false;
    m_state.m_emu->tick(input);
    shouldBreak |= m_state.m_emu->wantBreakpoint();
    const auto cyclesTillNext = m_state.m_emu->getCyclesUntilNextInstruction();
    shouldBreak |= m_state.m_emu->getPC() == m_state.m_debugSettings.m_breakOnPC;
    const auto currentOp = m_state.m_emu->getCurrentOpCode();
    shouldBreak |=
        currentOp.m_addr == (currentOp.m_prefixed ? m_state.m_debugSettings.m_breakOnOpCodePrefixed
                                                  : m_state.m_debugSettings.m_breakOnOpCode);
    if (m_state.m_emu->getLastWrittenAddr() == m_state.m_debugSettings.m_breakOnWriteAddr) {
        shouldBreak = true;
        if (cyclesTillNext == 0) {
            m_state.m_emu->getLastWrittenAddr() = -2;
        }
    }

    if (shouldBreak && cyclesTillNext == 1) {
        log_info("Debug Break!");
        m_state.m_isPaused = true;
    }
}

} // namespace ez