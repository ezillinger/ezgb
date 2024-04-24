#pragma once

#include "Base.h"
#include "Emulator.h"

namespace ez {

struct DebugSettings {
    //int m_breakOnPC = 0xc000;
    int m_breakOnPC = -1;
    int m_breakOnOpCode = -1;
    int m_breakOnOpCodePrefixed = -1;
};

struct AppState {
    bool m_isPaused = false;
    bool m_singleStep = false;
    std::unique_ptr<Cart> m_cart;
    std::unique_ptr<Emulator> m_emu;
    DebugSettings m_debugSettings{};
};

struct OpLine {
    int m_addr;
    OpCodeInfo m_info;
};

}