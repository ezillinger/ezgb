#include "Test.h"

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

    bool Tester::test_all() {
        log_info("Running test suite");
        auto success = true;

        success &= test_flags();
        success &= test_regs();
        success &= test_inc_dec();
        success &= test_push_pop();


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