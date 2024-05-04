#include "Base.h"
#include "Gui.h"
#include "Runner.h"
#include "Test.h"
#include "Window.h"

int main(int, char**) {
    using namespace ez;

    auto t = Tester{};
    t.test_all();

    log_info("CurrentDir: {}", fs::current_path().string().c_str());
    auto romStartsWith = "tetris";
    auto romPath = "./roms/cpu_instrs.gb"s;
    for (auto& romFile : fs::recursive_directory_iterator("./roms/")) {
        if (romFile.path().filename().string().starts_with(romStartsWith) &&
            romFile.path().extension() == ".gb") {
            romPath = romFile.path().string();
            break;
        }
    }

    auto state = AppState{};
    state.m_cart = std::make_unique<Cart>(Cart::load_from_disk(romPath));
    state.m_emu = std::make_unique<Emulator>(*state.m_cart);

    auto window = Window{"CoronaBoy"};
    auto gui = Gui(state);
    auto runner = Runner(state);
    bool shouldExit = false;

    while (true) {
        shouldExit |= window.run([&]() {
            const auto input = gui.handle_keyboard();
            while (RunResult::CONTINUE ==
                   runner.tick(input, [&](auto span) { window.push_audio(span); })) {
                // run emu logic
            }

            gui.draw();
            shouldExit |= gui.should_exit();
        });
        if (shouldExit) {
            break;
        }
    }

    return 0;
}

#if EZ_MSVC
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return main(__argc, __argv); }
#endif