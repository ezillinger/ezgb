#include "Base.h"
#include "Emulator.h"
#include "libs/imgui/imgui.h"

namespace ez {
class EmuGui {
  public:
    EmuGui(Emulator& emu) : m_emu(emu){};
    void drawGui();
    bool shouldExit() const { return m_shouldExit; };

  private:
    void drawToolbar();
    void drawRegisters();
    void drawSettings();

    Emulator& m_emu;
    bool m_shouldExit = false;
};
} // namespace ez