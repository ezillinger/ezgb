#include "Emulator.h"

int main() {
    using namespace ez;

    //const auto romPath = "C:\\git\\CoronaBoy\\roms\\DMG_ROM.bin";
    log_info("CurrentDir: {}", fs::current_path().c_str());
    const auto romPath = "./roms/bootix_dmg.bin";
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