#include "Base.h"
#include "AppState.h"
#include "libs/imgui/imgui.h"

namespace ez {

class Gui {
  public:
    Gui(AppState& state);

    void handleKeyboard();

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

    void clearCache();

    AppState& m_state;
    std::vector<fs::path> m_romsAvail;

    bool m_shouldExit = false;
    bool m_showDemoWindow = false;
    bool m_followPC = true;

    std::vector<OpLine> m_opCache;
};
} // namespace ez