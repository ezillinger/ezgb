#include "Base.h"
#include "Bootrom.h"
#include "Gui.h"
#include "Runner.h"
#include "Test.h"
#include "Window.h"
#include <thread>

#if EZ_WASM
static std::function<void()> MainLoopForEmscriptenP;
static void MainLoopForEmscripten() { MainLoopForEmscriptenP(); }
    #define EMSCRIPTEN_MAINLOOP_BEGIN MainLoopForEmscriptenP = [&]()
    #define EMSCRIPTEN_MAINLOOP_END                                                                \
        ;                                                                                          \
        emscripten_set_main_loop(MainLoopForEmscripten, 0, true)
#else
    #define EMSCRIPTEN_MAINLOOP_BEGIN
    #define EMSCRIPTEN_MAINLOOP_END
#endif

int main(int, char**) {
    using namespace ez;

    log_info("CurrentDir: {}", fs::current_path().string().c_str());

    auto t = Tester{};
    t.test_all();

    auto state = AppState{};

    static constexpr bool loadRomFromDisk = true;
    if (loadRomFromDisk) {
        log_info("Looking for ROMs");
        auto romStartsWith = "dmg-acid";
        auto romPath = "./roms/cpu_instrs.gb"s;
        for (auto& romFile : fs::recursive_directory_iterator("./roms/")) {
            if (romFile.path().filename().string().starts_with(romStartsWith) &&
                romFile.path().extension() == ".gb") {
                romPath = romFile.path().string();
                break;
            }
        }
        state.m_cart = std::make_unique<Cart>(Cart::load_from_disk(romPath));
    } else {
        log_info("Loading Windsor Road");
        state.m_cart = std::make_unique<Cart>(SAMPLE_ROM);
    }

    log_info("Created Cart");
    state.m_emu = std::make_unique<Emulator>(*state.m_cart);
    log_info("Created Emu");

    auto window = Window{"ezgb"};
    auto gui = Gui(state);
    auto runner = Runner(state);
    bool shouldExit = false;

#if EZ_WASM
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!shouldExit)
#endif
    {
        shouldExit |= window.run([&]() {
            const auto input = gui.handle_keyboard();
            while (RunResult::CONTINUE ==
                   runner.tick(input, [&](auto span) { window.push_audio(span); })) {
                // run emu logic
            }

            gui.draw();
            shouldExit |= gui.should_exit();
        });
    }
    EMSCRIPTEN_MAINLOOP_END;

    return 0;
}

#if EZ_MSVC
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return main(__argc, __argv); }
#endif