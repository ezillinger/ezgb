#include "AppState.h"
#include "Base.h"
#include "Window.h"
#include "ThirdParty_ImGui.h"
#include "Texture.h"

namespace ez {

void imguiImage(const Tex2D& tex, const ImVec2& imageSize);

class Gui {
  public:
    Gui(AppState& state);
    ~Gui();
    EZ_DECLARE_COPY_MOVE(Gui, delete, delete);

    InputState handle_keyboard();
    void draw();
    bool should_exit() const { return m_shouldExit; };

  private:

    void configure_ImGui();

    void put_next_window(const float2& posRowsCols, const float2& dimsRowsCols);
    static ImGuiWindowFlags getWindowFlags();

    void draw_toolbar();
    void draw_registers();
    void draw_settings();
    void draw_console();
    void draw_instructions();
    void draw_display();
    void draw_ppu();

    void update_rom_list();
    void update_op_cache();

    void clear_cache();
    void reset_emulator();

    AppState& m_state;
    std::vector<fs::path> m_romsAvail;

    bool m_shouldExit = false;
    bool m_showDemoWindow = false;
    bool m_followPC = true;

    bool m_prevWasPaused = false;

    std::vector<OpLine> m_opCache;

    bool m_ppuDisplayWindow = false; // otherwise BG

    std::string m_lastRom = "";

    enum class Textures {
        DISPLAY,
        BG,
        VRAM
    };

    Tex2D m_displayTex{int2{PPU::DISPLAY_WIDTH, PPU::DISPLAY_HEIGHT}};
    Tex2D m_bgWindowTex{int2{PPU::BG_WINDOW_DIM_XY, PPU::BG_WINDOW_DIM_XY}};
    Tex2D m_vramTex{int2{PPU::VRAM_DEBUG_FB_WIDTH, PPU::VRAM_DEBUG_FB_HEIGHT} };
};
} // namespace ez