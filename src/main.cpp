#include "Emulator.h"
#include "Test.h"

int main() {
    using namespace ez;

    auto t = Tester{};
    t.test_all();

    //const auto romPath = "C:\\git\\CoronaBoy\\roms\\DMG_ROM.bin";
    log_info("CurrentDir: {}", fs::current_path().c_str());
    const auto romPath = "./roms/tetris.gb";
    auto cart = Cart{ romPath };

    Emulator emu{cart};
    while (true) {
        const auto stop = emu.tick();
        if (stop) {
            break;
        }
    }

    return 0;
}