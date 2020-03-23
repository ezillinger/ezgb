#include "Emulator.h"

namespace ez {

    Emulator::Emulator(Cart& cart) 
        : m_cart(cart)
    {
    }

    InstructionResult Emulator::handleInstructionXOR8(uint8_t instruction) {
        uint8_t regValue = 0;
        switch (instruction) {
            case 0xAF: regValue = m_regA; break;
            case 0xA8: regValue = getRegB(); break;
            case 0xA9: regValue = getRegC(); break;
            case 0xAA: regValue = getRegD(); break;
            case 0xAB: regValue = getRegE(); break;
            case 0xAC: regValue = getRegH(); break;
            case 0xAD: regValue = getRegL(); break;
            default: EZ_FAIL();
        }
            EZ_INFO("XOR");
            m_regA ^= regValue;
            m_regFlags = 0;
            if (m_regA == 0) {
                setFlag(Flag::ZERO);
            }
            return InstructionResult{ 1, 4 };
    }

    // includes the 4 cycles and 1 byte from the CB opcode itself
    InstructionResult Emulator::handleInstructionCB(uint8_t inst8) {
        const auto isBitInstruction = (inst8 >= 0x40 && inst8 < 0x80);
        enum class RegisterOrder {
            B = 0,
            C,
            D,
            E,
            H,
            L,
            HL,
            A
        };
        if (isBitInstruction) {
            const auto bitInstrIdx = inst8 - 0x40;
            const auto bitNo = (bitInstrIdx) / 8;
            const auto reg = RegisterOrder{ bitInstrIdx - bitNo * 8 };
            const uint8_t numCycles = reg == RegisterOrder::HL ? 16 : 8;
            return InstructionResult{ 2, numCycles};
        }

        EZ_FAIL();
    }

    InstructionResult Emulator::handleInstruction(uint32_t instruction) {

        const auto inst8 = static_cast<uint8_t>( instruction & 0x000000FF);
        //const auto inst8 = *reinterpret_cast<uint8_t*>(&instruction);
        const auto d16 = static_cast<uint16_t>((instruction >> 8) & 0x0000FFFF);
        const auto d8 = static_cast<uint8_t>((instruction >> 8) & 0x000000FF);

        switch (inst8) {
            case 0x00: // NOP
                EZ_INFO("NOP");
                return InstructionResult{ 1, 4 };
            case 0x21: // LDD HL, d16
                EZ_INFO("LD HL, {:x}", d16);
                m_regHL = d16;
                return InstructionResult{ 3, 12 };
            case 0x31: // LD SP, d16
                EZ_INFO("LD SP, {:x}", d16);
                m_regSP = d16;
                return InstructionResult{ 3, 12 };
            case 0x32: // LDD (HL), A
                EZ_INFO("LDD (HL), A");
                write8(m_regHL, m_regA);
                --m_regHL;
                return InstructionResult{ 1, 8 };

            // 8 bit xors
            case 0xA8: [[fallthrough]];
            case 0xA9: [[fallthrough]];
            case 0xAA: [[fallthrough]];
            case 0xAB: [[fallthrough]];
            case 0xAC: [[fallthrough]];
            case 0xAD: [[fallthrough]];
            case 0xAF: return handleInstructionXOR8(inst8);

            case 0xCB: return handleInstructionCB(d8);
            default: 
                EZ_ERROR("UNKNOWN OPCODE: {:x}", inst8);
                EZ_FAIL();
        }
    }

    bool Emulator::getFlag(Flag flag) {
        return ((0x1 << static_cast<uint8_t>(flag)) & m_regFlags) != 0x0;
    }

    void Emulator::setFlag(Flag flag) {
        m_regFlags |= (0x1 << static_cast<uint8_t>(flag));
    }
    void Emulator::clearFlag(Flag flag) {
        m_regFlags &= ~(0x1 << static_cast<uint8_t>(flag));
    }

    bool Emulator::tick() {

        if (m_cyclesToWait == 0) {
            auto instruction = read32(m_regPC);
            const auto result = handleInstruction(instruction);
            m_regPC += result.m_size;
            m_cyclesToWait = result.m_cycles;
        }
        --m_cyclesToWait;

        return m_stop;
    }

    AddrInfo Emulator::getAddressInfo(uint16_t addr) {

        if (addr < 0x3FFF) {
            return { MemoryBank::ROM_0, 0 };
        }
        else if(addr >= 0xC000 && addr <= 0xCFFF){
            return { MemoryBank::WRAM_0, 0xC000 };
        }
        else if (addr >= 0x8000 && addr <= 0x9FFF) {
            return { MemoryBank::VRAM, 0x8000 };
        }
        else {
            EZ_FAIL();
        }

    }

    void Emulator::write8(uint16_t addr, uint8_t value) {
        const auto addrInfo = getAddressInfo(addr);
        switch (addrInfo.m_bank) {
            case MemoryBank::ROM_0:
                EZ_FAIL();
            case MemoryBank::WRAM_0:
                m_ram[addr - addrInfo.m_baseAddr] = value;
                break;
            case MemoryBank::VRAM:
                m_vram[addr - addrInfo.m_baseAddr] = value;
                break;
            default:
                EZ_FAIL();
                break;
        }
    }


    uint32_t Emulator::read32(uint16_t addr) {
        const auto addrInfo = getAddressInfo(addr);
        switch (addrInfo.m_bank) {
            case MemoryBank::ROM_0:
                return m_cart.read32(addr);
            case MemoryBank::WRAM_0:
                return reinterpret_cast<uint32_t>(m_ram.data() + addr - addrInfo.m_baseAddr);
            case MemoryBank::VRAM:
                return reinterpret_cast<uint32_t>(m_vram.data() + addr - addrInfo.m_baseAddr);
            default:
                EZ_FAIL();
                break;
        }
    }
}