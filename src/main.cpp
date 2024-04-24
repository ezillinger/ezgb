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
    // const auto romPath = "./roms/cpu_instrs.gb";
    // const auto romPath = "./roms/tetris.gb";
    const auto romPath = "./roms/07-jr,jp,call,ret,rst.gb";

    auto state = AppState{};
    state.m_cart = std::make_unique<Cart>(romPath);
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
            gui.drawGui();
            shouldExit |= gui.shouldExit();
        });
        if (shouldExit) {
            break;
        }
    }

    return 0;
}