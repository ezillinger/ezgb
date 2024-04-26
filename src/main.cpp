#include "Base.h"
#include "Gui.h"
#include "Test.h"
#include "Window.h"
#include "Runner.h"

int main() {
    using namespace ez;

    auto t = Tester{};
    t.test_all();

    log_info("CurrentDir: {}", fs::current_path().c_str());
    auto romStartsWith = "tetris";
    auto romPath = "./roms/cpu_instrs.gb"s;
    for(auto& romFile : fs::directory_iterator("./roms/")){
        if(romFile.path().filename().string().starts_with(romStartsWith)){
            romPath = romFile.path().string();
            break;
        }
    }

    auto state = AppState{};
    state.m_cart = std::make_unique<Cart>(Cart::loadFromDisk(romPath));
    state.m_emu = std::make_unique<Emulator>(*state.m_cart);

    auto window = Window{"CoronaBoy"};
    auto gui = Gui(state);
    auto runner = Runner(state);
    bool shouldExit = false;

    

    while (true) {
        shouldExit |= window.run([&]() {
            gui.handleKeyboard();
            while (RunResult::CONTINUE == runner.tick()) {
                // run emu logic
            }

            glTexImage2D(GL_TEXTURE_2D, 0, GL_R, PPU::DISPLAY_WIDTH, PPU::DISPLAY_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, state.m_emu->getDisplayFramebuffer());
            gui.drawGui();
            shouldExit |= gui.shouldExit();
        });
        if (shouldExit) {
            break;
        }
    }

    return 0;
}