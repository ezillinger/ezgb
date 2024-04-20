#include "Base.h"
#include "Emulator.h"
#include "libs/imgui/imgui.h"

namespace ez {

struct AppState {
    std::unique_ptr<Cart> m_cart;
    std::unique_ptr<Emulator> m_emu;
};

class EmuGui {
  public:
    EmuGui(AppState& state);
    void drawGui();
    bool shouldExit() const { return m_shouldExit; };

  private:
    void drawToolbar();
    void drawRegisters();
    void drawSettings();
    void drawConsole();

    void updateRomList();

    AppState& m_state;
    std::vector<fs::path> m_romsAvail;
    bool m_shouldExit = false;
};
} // namespace ez