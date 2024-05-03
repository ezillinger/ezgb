#include "Test.h"
#include "Base.h"
#include "MiscOps.h"

namespace ez {

Tester::Tester() {}

Cart Tester::make_cart() {
    const auto zeros = std::vector<uint8_t>(64 * 1024ull);
    return Cart(zeros.data(), zeros.size());
}

Emulator Tester::make_emulator() {
    m_cart = std::make_unique<Cart>(make_cart());
    auto emu = Emulator{*m_cart};
    emu.m_reg = {};
    emu.m_reg.sp = 0xFFFE;
    return emu;
}

bool Tester::test_timer() {

    auto tacs = std::array<uint8_t, 4>{ 0b100, 0b101, 0b110, 0b111};
    auto multiples = std::array<int, 4>{1024, 16, 64, 256};

    for(size_t test = 0; test < 4; ++test){
        uint16_t sysclk = 0;
        const auto multiple = multiples[test];
        const auto tac = tacs[test];
        for (int i = 0; i < UINT16_MAX / multiple; ++i) {
            for (int j = 0; j < multiple; ++j) {
                if(is_tima_increment(sysclk, sysclk + 1, tac, tac)){
                    ez_assert(multiple - 1 == j);
                } else {
                    ez_assert(multiple - 1 != j);
                }
                ++sysclk;
            }
        }
    }

    return true;
}

bool Tester::test_ppu() {
    const std::array<uint8_t, PPU::BYTES_PER_TILE_COMPRESSED> tile{0x3C, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
                                       0x7E, 0x5E, 0x7E, 0x0A, 0x7C, 0x56, 0x38, 0x7C};
    const std::array<uint8_t, PPU::TILE_DIM_XY * PPU::TILE_DIM_XY> expected{
        0b00, 0b10, 0b11, 0b11, 0b11, 0b11, 0b10, 0b00, 0b00, 0b11, 0b00, 0b00, 0b00,
        0b00, 0b11, 0b00, 0b00, 0b11, 0b00, 0b00, 0b00, 0b00, 0b11, 0b00, 0b00, 0b11,
        0b00, 0b00, 0b00, 0b00, 0b11, 0b00, 0b00, 0b11, 0b01, 0b11, 0b11, 0b11, 0b11,
        0b00, 0b00, 0b01, 0b01, 0b01, 0b11, 0b01, 0b11, 0b00, 0b00, 0b11, 0b01, 0b11,
        0b01, 0b11, 0b10, 0b00, 0b00, 0b10, 0b11, 0b11, 0b11, 0b10, 0b00, 0b00};
    std::array<uint8_t, 64> output;
    PPU::render_tile(tile.data(), output.data(), PPU::TILE_DIM_XY);
    for (auto i = 0; i < 64; ++i) {
        ez_assert(expected[i] == output[i]);
    }

    return true;
}

bool Tester::test_io_reg() {
    auto emu = make_emulator();

    const auto last_addr = [](auto& arr) -> uint8_t* { return &arr[std::size(arr) - 1]; };

    ez_assert(emu.dbg_get_io_ptr(+IOAddr::P1_JOYP) == &emu.m_ioReg.m_joypad);

    ez_assert(emu.dbg_get_io_ptr(+IOAddr::SB) == &emu.m_ioReg.m_serialData);
    ez_assert(emu.dbg_get_io_ptr(+IOAddr::SC) == &emu.m_ioReg.m_serialControl);

    ez_assert(emu.dbg_get_io_ptr(+IOAddr::DIV) == &emu.m_ioReg.m_timerDivider);
    ez_assert(emu.dbg_get_io_ptr(+IOAddr::TIMA) == &emu.m_ioReg.m_tima);
    ez_assert(emu.dbg_get_io_ptr(+IOAddr::TMA) == &emu.m_ioReg.m_tma);
    ez_assert(emu.dbg_get_io_ptr(+IOAddr::TAC) == &emu.m_ioReg.m_tac);

    ez_assert(emu.dbg_get_io_ptr(+IOAddr::IF) == reinterpret_cast<uint8_t*>(&emu.m_ioReg.m_if));

    ez_assert(emu.dbg_get_io_ptr(+IOAddr::NR10) == emu.m_ioReg.m_audio);
    ez_assert(emu.dbg_get_io_ptr(0xFF26) == last_addr(emu.m_ioReg.m_audio));

    ez_assert(emu.dbg_get_io_ptr(+IOAddr::WaveRAMBegin) == emu.m_ioReg.m_wavePattern);
    ez_assert(emu.dbg_get_io_ptr(0xFF3F) == last_addr(emu.m_ioReg.m_wavePattern));

    ez_assert(emu.dbg_get_io_ptr(+IOAddr::LCDC) == reinterpret_cast<uint8_t*>(&emu.m_ioReg.m_lcd));
    // todo, test individual LCD register layout

    ez_assert(emu.dbg_get_io_ptr(+IOAddr::VBK) == &emu.m_ioReg.m_vramBankSelect);
    ez_assert(emu.dbg_get_io_ptr(0xFF50) ==
              reinterpret_cast<uint8_t*>(&emu.m_ioReg.m_bootromDisabled));

    ez_assert(emu.dbg_get_io_ptr(0xFF51) == emu.m_ioReg.m_vramDMA);
    ez_assert(emu.dbg_get_io_ptr(0xFF55) == last_addr(emu.m_ioReg.m_vramDMA));

    ez_assert(emu.dbg_get_io_ptr(0xFF68) == emu.m_ioReg.m_bgObjPalettes);
    ez_assert(emu.dbg_get_io_ptr(0xFF6B) == last_addr(emu.m_ioReg.m_bgObjPalettes));

    ez_assert(emu.dbg_get_io_ptr(0xFF70) == &emu.m_ioReg.m_wramBankSelect);

    ez_assert(emu.dbg_get_io_ptr(0xFF76) == &emu.m_ioReg.m_pcm12);
    ez_assert(emu.dbg_get_io_ptr(0xFF77) == &emu.m_ioReg.m_pcm34);

    ez_assert(emu.dbg_get_io_ptr(0xFF80) == emu.m_ioReg.m_hram);

    emu.write_addr_16(0xFF81, 0xABCD);
    ez_assert(emu.readAddr16(0xFF81) == 0xABCD);

    ez_assert(emu.dbg_get_io_ptr(0xFFFF) == reinterpret_cast<uint8_t*>(&emu.m_ioReg.m_ie));

    emu.write_addr(+IOAddr::IF, 0b00010101);
    ez_assert(emu.read_addr(+IOAddr::IF) == 0b00010101);
    ez_assert(emu.m_ioReg.m_if.vblank);
    ez_assert(emu.m_ioReg.m_if.data & 0b1 << +Interrupts::VBLANK);

    ez_assert(!emu.m_ioReg.m_if.lcd);
    ez_assert(!(emu.m_ioReg.m_if.data & 0b1 << +Interrupts::LCD));

    ez_assert(emu.m_ioReg.m_if.timer);
    ez_assert(emu.m_ioReg.m_if.data & 0b1 << +Interrupts::TIMER);

    ez_assert(!emu.m_ioReg.m_if.serial);
    ez_assert(!(emu.m_ioReg.m_if.data & 0b1 << +Interrupts::SERIAL));

    ez_assert(emu.m_ioReg.m_if.joypad);
    ez_assert(emu.m_ioReg.m_if.data & 0b1 << +Interrupts::JOYPAD);

    emu.write_addr(+IOAddr::IE, 0b0001'0101);
    ez_assert(emu.read_addr(+IOAddr::IE) == 0b0001'0101);

    ez_assert(emu.m_ioReg.m_ie.vblank);
    ez_assert(emu.m_ioReg.m_ie.data & 0b1 << +Interrupts::VBLANK);

    ez_assert(!emu.m_ioReg.m_ie.lcd);
    ez_assert(!(emu.m_ioReg.m_ie.data & 0b1 << +Interrupts::LCD));

    ez_assert(emu.m_ioReg.m_ie.timer);
    ez_assert(emu.m_ioReg.m_ie.data & 0b1 << +Interrupts::TIMER);

    ez_assert(!emu.m_ioReg.m_ie.serial);
    ez_assert(!(emu.m_ioReg.m_ie.data & 0b1 << +Interrupts::SERIAL));

    ez_assert(emu.m_ioReg.m_ie.joypad);
    ez_assert(emu.m_ioReg.m_ie.data & 0b1 << +Interrupts::JOYPAD);

    return true;
}

bool Tester::test_all() {
    log_info("Running test suite");
    auto success = true;

    success &= test_flags();
    success &= test_regs();
    success &= test_io_reg();
    success &= test_inc_dec();
    success &= test_push_pop();
    success &= test_call_ret();
    success &= test_cart();
    success &= test_ppu();
    success &= test_timer();

    if (success) {
        log_info("All tests passed!");
    }
    return success;
}

bool Tester::test_regs() {
    auto emu = make_emulator();
    emu.m_reg.hl = 0xFFFF;
    ez_assert(emu.m_reg.h == 0xFF);
    ez_assert(emu.m_reg.l == 0xFF);

    emu.m_reg.hl = 0x0000;
    ez_assert(emu.m_reg.h == 0x00);
    ez_assert(emu.m_reg.l == 0x00);

    emu.m_reg.hl = 0xFF00;
    ez_assert(emu.m_reg.h == 0xFF);
    ez_assert(emu.m_reg.l == 0x00);

    return true;
}

bool Tester::test_cart() {
    auto cart = make_cart();
    const auto baseAddr = cart.m_data.data();
    cart.m_cartType = CartType::MBC1;

    intptr_t offset = 0;
    cart.m_mbc1State.m_romBankSelect = 0;
    offset = cart.get_rom_ptr(0x7FFF) - baseAddr;
    ez_assert(offset == 0x7FFF);

    cart.m_mbc1State.m_romBankSelect = 1;
    offset = cart.get_rom_ptr(0x7FFF) - baseAddr;
    ez_assert(offset == 0x7FFF);

    return true;
}

bool Tester::test_flags() {
    auto emu = make_emulator();
    emu.m_reg.a = 0;
    emu.m_reg.f = 0;
    for (auto flagSet : {Flag::ZERO, Flag::CARRY, Flag::HALF_CARRY, Flag::NEGATIVE}) {
        emu.set_flag(flagSet);
        ez_assert(emu.m_reg.a == 0);
        for (auto flagCheck : {Flag::ZERO, Flag::CARRY, Flag::HALF_CARRY, Flag::NEGATIVE}) {
            ez_assert(emu.get_flag(flagCheck) == (flagSet == flagCheck));
        }
        emu.clear_flag(flagSet);
        ez_assert(emu.m_reg.a == 0);
    }

    emu.m_reg.f = 0;
    emu.write_R16Stack(R16Stack::AF, 0xFFFF);
    for (auto flagSet : {Flag::ZERO, Flag::CARRY, Flag::HALF_CARRY, Flag::NEGATIVE}) {
        ez_assert(emu.get_flag(flagSet));
    }
    ez_assert((emu.m_reg.f & 0x0F) == 0);

    return true;
}

bool Tester::test_call_ret() {
    auto emu = make_emulator();

    auto makeOpBytesU16 = [](OpCode oc, uint16_t u16) {
        return uint32_t(+oc) | uint32_t(u16) << 8;
    };
    auto makeOpBytes = [](OpCode oc) { return uint32_t(+oc); };
    emu.m_reg.sp = 0xFFFE;
    emu.m_reg.pc = 0xABCD;
    auto result = emu.handle_instr(makeOpBytesU16(OpCode::CALL_a16, 0x1234));
    ez_assert(result.m_newPC == 0x1234);
    result = emu.handle_instr(makeOpBytes(OpCode::RET));
    ez_assert(result.m_newPC == 0xABCD + 3);
    ez_assert(emu.m_reg.sp == 0xFFFE);

    return true;
}

bool Tester::test_push_pop() {
    auto emu = make_emulator();

    auto makeOpBytes = [](OpCode oc) { return uint32_t(+oc); };
    emu.m_reg.bc = 0xABCD;
    emu.handle_instr(makeOpBytes(OpCode::PUSH_BC));
    ez_assert(emu.m_reg.bc == 0xABCD);
    emu.m_reg.bc = 0x0000;
    emu.handle_instr(makeOpBytes(OpCode::POP_BC));
    ez_assert(emu.m_reg.bc == 0xABCD);

    emu.m_reg.hl = 0xFEDB;
    emu.handle_instr(makeOpBytes(OpCode::PUSH_HL));
    emu.m_reg.hl = 0x0000;
    emu.handle_instr(makeOpBytes(OpCode::POP_HL));
    ez_assert(emu.m_reg.hl == 0xFEDB);

    return true;
}

bool Tester::test_inc_dec() {
    auto emu = make_emulator();

    ez_assert(emu.get_flag(Flag::ZERO) == false);
    auto makeOpBytes = [](OpCode oc) { return uint32_t(+oc); };
    ez_assert(emu.m_reg.a == 0);
    emu.handle_instr(makeOpBytes(OpCode::INC_A));

    ez_assert(emu.m_reg.a == 1);
    ez_assert(emu.get_flag(Flag::ZERO) == false);
    emu.handle_instr(makeOpBytes(OpCode::INC_A));
    ez_assert(emu.m_reg.a == 2);
    ez_assert(emu.get_flag(Flag::ZERO) == false);
    ez_assert(emu.get_flag(Flag::HALF_CARRY) == false);
    ez_assert(emu.get_flag(Flag::NEGATIVE) == false);

    emu.handle_instr(makeOpBytes(OpCode::DEC_A));
    ez_assert(emu.m_reg.a == 1);
    ez_assert(emu.get_flag(Flag::ZERO) == false);

    emu.handle_instr(makeOpBytes(OpCode::DEC_A));
    ez_assert(emu.m_reg.a == 0);
    ez_assert(emu.get_flag(Flag::ZERO) == true);
    ez_assert(emu.get_flag(Flag::NEGATIVE) == true);

    // dec wraparound
    emu.handle_instr(makeOpBytes(OpCode::DEC_A));
    ez_assert(emu.get_flag(Flag::ZERO) == false);
    ez_assert(emu.get_flag(Flag::HALF_CARRY) == true);
    ez_assert(emu.m_reg.a == 255);

    // inc wraparound
    emu.clear_flag(Flag::ZERO);
    emu.clear_flag(Flag::HALF_CARRY);
    emu.handle_instr(makeOpBytes(OpCode::INC_A));
    ez_assert(emu.m_reg.a == 0);
    ez_assert(emu.get_flag(Flag::ZERO) == true);
    ez_assert(emu.get_flag(Flag::HALF_CARRY) == true);

    return true;
}
} // namespace ez