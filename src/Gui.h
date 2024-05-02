#include "AppState.h"
#include "Base.h"
#include "Window.h"

namespace ez {

class Gui {
  public:
    Gui(AppState& state);
    ~Gui();
    EZ_DEFINE_COPY_MOVE(Gui, delete, delete);

    JoypadState handleKeyboard();
    void drawGui();
    bool shouldExit() const { return m_shouldExit; };

  private:
    void drawToolbar();
    void drawRegisters();
    void drawSettings();
    void drawConsole();
    void drawInstructions();
    void drawDisplay();
    void drawPPU();

    void updateRomList();
    void updateOpCache();

    void clearCache();
    void resetEmulator();

    AppState& m_state;
    std::vector<fs::path> m_romsAvail;

    bool m_shouldExit = false;
    bool m_showDemoWindow = false;
    bool m_followPC = true;

    bool m_prevWasPaused = false;

    std::vector<OpLine> m_opCache;

    bool m_ppuDisplayWindow = false; // otherwise BG

    std::string m_lastRom = "";

    // todo, RAII
    enum class Textures {
        DISPLAY,
        BG,
        VRAM
    };
    static constexpr auto numTextures = 3;
    std::array<GLuint, numTextures> m_texHandles;
    std::array<int2, numTextures> m_texDims{
        int2{PPU::DISPLAY_WIDTH, PPU::DISPLAY_HEIGHT},            //
        int2{PPU::BG_DIM_XY, PPU::BG_DIM_XY},                     //
        int2{PPU::VRAM_DEBUG_FB_WIDTH, PPU::VRAM_DEBUG_FB_HEIGHT} //
    };
};
} // namespace ez