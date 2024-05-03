#include "Runner.h"

namespace ez {

RunResult ez::Runner::tick(const InputState& input) {
    auto ret = RunResult::CONTINUE;
    if (m_state.m_isPaused) {
        if (m_state.m_stepToNextInstr) {
            while (true) {
                tick_emu_once(input);
                if (m_state.m_emu->executed_instr_this_cycle()) {
                    break;
                }
            }
            m_state.m_stepToNextInstr = false;
        }
        else if(m_state.m_stepOneCycle){
            tick_emu_once(input);
            m_state.m_stepOneCycle = false;
        }
        ret = RunResult::DRAW;
    } else {
        tick_emu_once(input);
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

void Runner::tick_emu_once(const InputState& input) {
    bool shouldBreak = false;
    m_state.m_emu->tick(input);
    shouldBreak |= m_state.m_emu->want_breakpoint();
    shouldBreak |= m_state.m_emu->get_pc() == m_state.m_debugSettings.m_breakOnPC;
    const auto currentOp = m_state.m_emu->get_current_opcode();
    shouldBreak |=
        currentOp.m_addr == (currentOp.m_prefixed ? m_state.m_debugSettings.m_breakOnOpCodePrefixed
                                                  : m_state.m_debugSettings.m_breakOnOpCode);
    if (m_state.m_emu->get_last_written_addr() == m_state.m_debugSettings.m_breakOnWriteAddr) {
        shouldBreak = true;
        if (m_state.m_emu->executed_instr_this_cycle()) {
            m_state.m_emu->get_last_written_addr() = -2;
        }
    }

    if (shouldBreak && m_state.m_emu->executed_instr_this_cycle()) {
        log_info("Debug Break!");
        m_state.m_isPaused = true;
    }
}

} // namespace ez