#include "Emulator.h"
#include "Test.h"
#include "Window.h"
#include "Gui.h"

int main() {
    using namespace ez;

    auto t = Tester{};
    t.test_all();

    log_info("CurrentDir: {}", fs::current_path().c_str());
    //const auto romPath = "./roms/cpu_instrs.gb";
    //const auto romPath = "./roms/tetris.gb";
    const auto romPath = "./roms/07-jr,jp,call,ret,rst.gb";

    auto state = AppState{};
    state.m_isPaused = true;
    state.m_cart = std::make_unique<Cart>(romPath);
    state.m_emu = std::make_unique<Emulator>(*state.m_cart);

          auto window = Window{"CoronaBoy"};
    auto gui = Gui(state);
    bool shouldExit = false;

    while (true) {
        shouldExit |= window.run([&]() {
            auto sw = Stopwatch();
            // todo, figure out how to properly allow this to run at a different frequency than vsync
            const auto frameTime = 16.6666ms;
            const auto timeToDraw = 0.1ms;
            while(sw.elapsed() < (frameTime - timeToDraw)){
                if(state.m_isPaused){
                    if(state.m_singleStep){
                        shouldExit |= state.m_emu->tick();
                        state.m_singleStep = false;
                    }
                    break;
                }
                else{
                    shouldExit |= state.m_emu->tick();
                }
            }
            gui.drawGui();
            shouldExit |= gui.shouldExit();
        });
        if (shouldExit) {
            break;
        }
    }

    return 0;
}