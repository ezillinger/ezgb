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
    EmuGui(AppState& state) : m_state(state){};
    void drawGui();
    bool shouldExit() const { return m_shouldExit; };

  private:
    void drawToolbar();
    void drawRegisters();
    void drawSettings();

    AppState& m_state;
    bool m_shouldExit = false;
};
} // namespace ez