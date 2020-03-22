#include "Emulator.h"

int main() {

    const auto romPath = "C:\rom\sbm.gb";
    EZ_ENSURE(std::filesystem::exists(romPath));

    ez::Emulator emu{romPath};

    getchar();
    return 0;
}