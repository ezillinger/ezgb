#include "Test.h"
#include "Base.h"

namespace ez {

    Tester::Tester(){
        const auto zeros = std::vector<uint8_t>(256ull);
        m_cart = std::make_unique<Cart>(zeros.data(), zeros.size());
    }

    Emulator Tester::make_emulator() {
        auto emu = Emulator{ *m_cart };
        emu.m_reg.sp = 0xFFFE;
        return emu;
    }

    bool Tester::test_io_reg() { 
        auto emu = make_emulator();
        auto& io = emu.m_io;

        const auto last_addr = [](auto& arr) -> uint8_t* { return &arr[std::size(arr) - 1]; };

        EZ_ASSERT(io.getMemPtrRW(0xFF00) == &io.m_reg.m_joypad);

        EZ_ASSERT(io.getMemPtrRW(0xFF01) == io.m_reg.m_serial);
        EZ_ASSERT(io.getMemPtrRW(0xFF02) == last_addr(io.m_reg.m_serial));

        EZ_ASSERT(io.getMemPtrRW(0xFF04) == io.m_reg.m_timerDivider);
        EZ_ASSERT(io.getMemPtrRW(0xFF07) == last_addr(io.m_reg.m_timerDivider));

        EZ_ASSERT(io.getMemPtrRW(0xFF10) == io.m_reg.m_audio);
        EZ_ASSERT(io.getMemPtrRW(0xFF26) == last_addr(io.m_reg.m_audio));

        EZ_ASSERT(io.getMemPtrRW(0xFF30) == io.m_reg.m_wavePattern);
        EZ_ASSERT(io.getMemPtrRW(0xFF3F) == last_addr(io.m_reg.m_wavePattern));

        EZ_ASSERT(io.getMemPtrRW(0xFF40) == reinterpret_cast<uint8_t*>(&io.m_reg.m_lcd));
        // todo, test individual LCD register layout

        EZ_ASSERT(io.getMemPtrRW(0xFF4F) == &io.m_reg.m_vramBankSelect);
        EZ_ASSERT(io.getMemPtrRW(0xFF50) == reinterpret_cast<uint8_t*>(&io.m_reg.m_bootromDisabled));

        EZ_ASSERT(io.getMemPtrRW(0xFF51) == io.m_reg.m_vramDMA);
        EZ_ASSERT(io.getMemPtrRW(0xFF55) == last_addr(io.m_reg.m_vramDMA));

        EZ_ASSERT(io.getMemPtrRW(0xFF68) == io.m_reg.m_bgObjPalettes);
        EZ_ASSERT(io.getMemPtrRW(0xFF6B) == last_addr(io.m_reg.m_bgObjPalettes));

        EZ_ASSERT(io.getMemPtrRW(0xFF70) == &io.m_reg.m_wramBankSelect);

        return true;
    }

    bool Tester::test_all() {
        log_info("Running test suite");
        auto success = true;

        success &= test_flags();
        success &= test_regs();
        success &= test_inc_dec();
        success &= test_push_pop();
        success &= test_io_reg();

        if(success){
            log_info("All tests passed!");
        }
        return success;
    }

    bool Tester::test_regs() { 
        auto emu = make_emulator();
        emu.m_reg.hl = 0xFFFF;
        EZ_ASSERT(emu.m_reg.h == 0xFF);
        EZ_ASSERT(emu.m_reg.l == 0xFF);
        
        emu.m_reg.hl = 0x0000;
        EZ_ASSERT(emu.m_reg.h == 0x00);
        EZ_ASSERT(emu.m_reg.l == 0x00);

        emu.m_reg.hl = 0xFF00;
        EZ_ASSERT(emu.m_reg.h == 0xFF);
        EZ_ASSERT(emu.m_reg.l == 0x00);

        return true;
    }

    bool Tester::test_flags() { 
        auto emu = make_emulator();
        emu.m_reg.a = 0;
        for (auto flagSet : {Flag::ZERO, Flag::CARRY, Flag::HALF_CARRY, Flag::NEGATIVE}) {
            emu.setFlag(flagSet);
            EZ_ASSERT(emu.m_reg.a == 0);
            for (auto flagCheck : {Flag::ZERO, Flag::CARRY, Flag::HALF_CARRY, Flag::NEGATIVE}) {
                EZ_ASSERT(emu.getFlag(flagCheck) == (flagSet == flagCheck));
            }
            emu.clearFlag(flagSet);
            EZ_ASSERT(emu.m_reg.a == 0);
        }

        emu.m_reg.f = 0;
        emu.writeR16Stack(R16Stack::AF, 0xFFFF);
        for (auto flagSet : {Flag::ZERO, Flag::CARRY, Flag::HALF_CARRY, Flag::NEGATIVE}) {
            EZ_ASSERT(emu.getFlag(flagSet));
        }
        EZ_ASSERT((emu.m_reg.f & 0x0F) == 0);

        return true;
    }

    bool Tester::test_push_pop() { 
        auto emu = make_emulator();

        auto makeOpBytes = [](OpCode oc) { return uint32_t(+oc); };
        emu.m_reg.bc = 0xABCD;
        emu.handleInstruction(makeOpBytes(OpCode::PUSH_BC));
        EZ_ASSERT(emu.m_reg.bc == 0xABCD);
        emu.m_reg.bc = 0x0000;
        emu.handleInstruction(makeOpBytes(OpCode::POP_BC));
        EZ_ASSERT(emu.m_reg.bc == 0xABCD);


        emu.m_reg.hl = 0xFEDB;
        emu.handleInstruction(makeOpBytes(OpCode::PUSH_HL));
        emu.m_reg.hl = 0x0000;
        emu.handleInstruction(makeOpBytes(OpCode::POP_HL));
        EZ_ASSERT(emu.m_reg.hl == 0xFEDB);

        return true;
    }

    bool Tester::test_inc_dec() { 
        auto emu = make_emulator();

        EZ_ASSERT(emu.getFlag(Flag::ZERO) == false);
        auto makeOpBytes = [](OpCode oc) { return uint32_t(+oc); };
        EZ_ASSERT(emu.m_reg.a == 0);
        emu.handleInstruction(makeOpBytes(OpCode::INC_A));

        EZ_ASSERT(emu.m_reg.a == 1);
        EZ_ASSERT(emu.getFlag(Flag::ZERO) == false);
        emu.handleInstruction(makeOpBytes(OpCode::INC_A));
        EZ_ASSERT(emu.m_reg.a == 2);
        EZ_ASSERT(emu.getFlag(Flag::ZERO) == false);
        EZ_ASSERT(emu.getFlag(Flag::HALF_CARRY) == false);
        EZ_ASSERT(emu.getFlag(Flag::NEGATIVE) == false);


        emu.handleInstruction(makeOpBytes(OpCode::DEC_A));
        EZ_ASSERT(emu.m_reg.a == 1);
        EZ_ASSERT(emu.getFlag(Flag::ZERO) == false);

        emu.handleInstruction(makeOpBytes(OpCode::DEC_A));
        EZ_ASSERT(emu.m_reg.a == 0);
        EZ_ASSERT(emu.getFlag(Flag::ZERO) == true);
        EZ_ASSERT(emu.getFlag(Flag::NEGATIVE) == true);

        // dec wraparound
        emu.handleInstruction(makeOpBytes(OpCode::DEC_A));
        EZ_ASSERT(emu.getFlag(Flag::ZERO) == false);
        EZ_ASSERT(emu.getFlag(Flag::HALF_CARRY) == true);
        EZ_ASSERT(emu.m_reg.a == 255);

        // inc wraparound
        emu.clearFlag(Flag::ZERO);
        emu.clearFlag(Flag::HALF_CARRY);
        emu.handleInstruction(makeOpBytes(OpCode::INC_A));
        EZ_ASSERT(emu.m_reg.a == 0);
        EZ_ASSERT(emu.getFlag(Flag::ZERO) == true);
        EZ_ASSERT(emu.getFlag(Flag::HALF_CARRY) == true);
        

        return true;
    }
}