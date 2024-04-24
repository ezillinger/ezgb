#include "Base.h"
#include "Emulator.h"
#include "libs/imgui/imgui.h"

namespace ez {

struct AppState {
    bool m_isPaused = false;
    bool m_singleStep = false;
    std::unique_ptr<Cart> m_cart;
    std::unique_ptr<Emulator> m_emu;
};

struct OpLine {
    int m_addr;
    OpCodeInfo m_info;
};

class Gui {
  public:
    Gui(AppState& state);
    void drawGui();
    bool shouldExit() const { return m_shouldExit; };

  private:
    void drawToolbar();
    void drawRegisters();
    void drawSettings();
    void drawConsole();
    void drawInstructions();
    void drawDisplay();

    void updateRomList();
    void updateOpCache();

    AppState& m_state;
    std::vector<fs::path> m_romsAvail;

    bool m_shouldExit = false;
    bool m_showDemoWindow = false;
    bool m_followPC = true;

    std::vector<OpLine> m_opCache;
};
} // namespace ez