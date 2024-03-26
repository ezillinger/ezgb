#include "Test.h"

namespace ez {

    void Tester::reset_emulator() {
        const auto zeros = std::vector<uint8_t>(256ull);
        m_cart = std::make_unique<Cart>(zeros.data(), zeros.size());
        m_emu = std::make_unique<Emulator>(*m_cart);
    }

    bool Tester::test_all() {
        log_info("Running test suite");
        auto success = true;

        success &= test_flags();
        success &= test_regs();
        success &= test_inc_dec();


        if(success){
            log_info("All tests passed!");
        }
        return success;
    }

    bool Tester::test_regs() { 
        reset_emulator();
        m_emu->m_reg.hl = 0xFFFF;
        EZ_ASSERT(m_emu->m_reg.h == 0xFF);
        EZ_ASSERT(m_emu->m_reg.l == 0xFF);
        
        m_emu->m_reg.hl = 0x0000;
        EZ_ASSERT(m_emu->m_reg.h == 0x00);
        EZ_ASSERT(m_emu->m_reg.l == 0x00);

        m_emu->m_reg.hl = 0xFF00;
        EZ_ASSERT(m_emu->m_reg.h == 0xFF);
        EZ_ASSERT(m_emu->m_reg.l == 0x00);

        return true;
    }

    bool Tester::test_flags() { 
        reset_emulator();
        m_emu->m_reg.a = 0;
        for (auto flagSet : {Flag::ZERO, Flag::CARRY, Flag::HALF_CARRY, Flag::NEGATIVE}) {
            m_emu->setFlag(flagSet);
            EZ_ASSERT(m_emu->m_reg.a == 0);
            for (auto flagCheck : {Flag::ZERO, Flag::CARRY, Flag::HALF_CARRY, Flag::NEGATIVE}) {
                EZ_ASSERT(m_emu->getFlag(flagCheck) == (flagSet == flagCheck));
            }
            m_emu->clearFlag(flagSet);
            EZ_ASSERT(m_emu->m_reg.a == 0);
        }

        return true;
    }

    bool Tester::test_inc_dec() { 
        reset_emulator();

        EZ_ASSERT(m_emu->getFlag(Flag::ZERO) == false);
        auto makeOpBytes = [](OpCode oc) { return uint32_t(+oc); };
        EZ_ASSERT(m_emu->m_reg.a == 0);
        m_emu->handleInstruction(makeOpBytes(OpCode::INC_A));

        EZ_ASSERT(m_emu->m_reg.a == 1);
        EZ_ASSERT(m_emu->getFlag(Flag::ZERO) == false);
        m_emu->handleInstruction(makeOpBytes(OpCode::INC_A));
        EZ_ASSERT(m_emu->m_reg.a == 2);
        EZ_ASSERT(m_emu->getFlag(Flag::ZERO) == false);
        EZ_ASSERT(m_emu->getFlag(Flag::HALF_CARRY) == false);
        EZ_ASSERT(m_emu->getFlag(Flag::NEGATIVE) == false);


        m_emu->handleInstruction(makeOpBytes(OpCode::DEC_A));
        EZ_ASSERT(m_emu->m_reg.a == 1);
        EZ_ASSERT(m_emu->getFlag(Flag::ZERO) == false);

        m_emu->handleInstruction(makeOpBytes(OpCode::DEC_A));
        EZ_ASSERT(m_emu->m_reg.a == 0);
        EZ_ASSERT(m_emu->getFlag(Flag::ZERO) == true);
        EZ_ASSERT(m_emu->getFlag(Flag::NEGATIVE) == true);

        // dec wraparound
        m_emu->handleInstruction(makeOpBytes(OpCode::DEC_A));
        EZ_ASSERT(m_emu->getFlag(Flag::ZERO) == false);
        EZ_ASSERT(m_emu->getFlag(Flag::HALF_CARRY) == true);
        EZ_ASSERT(m_emu->m_reg.a == 255);

        // inc wraparound
        m_emu->clearFlag(Flag::ZERO);
        m_emu->clearFlag(Flag::HALF_CARRY);
        m_emu->handleInstruction(makeOpBytes(OpCode::INC_A));
        EZ_ASSERT(m_emu->m_reg.a == 0);
        EZ_ASSERT(m_emu->getFlag(Flag::ZERO) == true);
        EZ_ASSERT(m_emu->getFlag(Flag::HALF_CARRY) == true);
        

        return true;
    }
}