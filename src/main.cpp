#include "Emulator.h"
#include "Test.h"
#include "GuiWindow.h"
#include "EmuGui.h"

int main() {
    using namespace ez;

    auto t = Tester{};
    t.test_all();

    log_info("CurrentDir: {}", fs::current_path().c_str());
    const auto romPath = "./roms/cpu_instrs.gb";
    //const auto romPath = "./roms/tetris.gb";
    auto cart = std::make_unique<Cart>( romPath );
    auto emu = std::make_unique<Emulator>(*cart);
    auto state = AppState{std::move(cart), std::move(emu)};

    auto window = GuiWindow{"CoronaBoy"};
    auto gui = EmuGui(state);
    bool shouldExit = false;

    while (true) {
        shouldExit |= window.run([&]() {
            auto sw = Stopwatch();
            // todo, figure out how to properly allow this to run at a different frequency than vsync
            const auto frameTime = 16.6666ms;
            const auto timeToDraw = 0.1ms;
            while(sw.elapsed() < (frameTime - timeToDraw)){
                shouldExit |= state.m_emu->tick();
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