#include "Emulator.h"
#include "OpCodes.h"

namespace ez {

    Emulator::Emulator(Cart& cart) 
        : m_cart(cart)
    {
    }

    InstructionResult Emulator::handleInstructionXOR8(uint8_t instruction) {
        uint8_t regValue = 0;
        switch (instruction) {
            case 0xA8: regValue = readReg8(Registers::B); break;
            case 0xA9: regValue = readReg8(Registers::C); break;
            case 0xAA: regValue = readReg8(Registers::D); break;
            case 0xAB: regValue = readReg8(Registers::E); break;
            case 0xAC: regValue = readReg8(Registers::H); break;
            case 0xAD: regValue = readReg8(Registers::L); break;
            case 0xAF: regValue = readReg8(Registers::A); break;
            default: EZ_FAIL();
        }
            log_info("XOR");
            auto aVal = readReg8(Registers::A);
            aVal ^= regValue;
            writeReg8(Registers::A, aVal);
            writeReg8(Registers::F, 0);
            if (aVal == 0) {
                setFlag(Flag::ZERO);
            }
            return InstructionResult{ 1, 4 };
    }

    // includes the 4 cycles and 1 byte from the CB opcode itself
    InstructionResult Emulator::handleInstructionCB(uint8_t inst8) {
        const auto isBitInstruction = (inst8 >= 0x40 && inst8 < 0x80);
        static const Registers regOrder[]= {
            Registers::B,
            Registers::C,
            Registers::D,
            Registers::E,
            Registers::H,
            Registers::L,
            Registers::HL,
            Registers::A
        };
        if (isBitInstruction) {
            const auto bitInstrIdx = inst8 - 0x40;
            const auto bitNo = (bitInstrIdx) / 8;
            const auto reg = regOrder[bitInstrIdx - bitNo * 8];
            const auto regSizeBytes = getRegisterSizeBytes(reg);
            bool isZero = false;
            if (regSizeBytes == 1) {
                const auto regVal = readReg8(reg);
                isZero = (regVal & (0x1 << bitNo)) == 0;
            }
            else {
                const auto regVal = readReg16(reg);
                isZero = (regVal & (0x1 << bitNo)) == 0;
            }
            isZero ? setFlag(Flag::ZERO) : clearFlag(Flag::ZERO);
            clearFlag(Flag::SUBTRACT);
            setFlag(Flag::HALF_CARRY);
            const uint8_t numCycles = regSizeBytes == 2 ? 16 : 8;
            return InstructionResult{ 2, numCycles};
        }

        EZ_FAIL();
    }

    constexpr uint8_t Emulator::getRegisterSizeBytes(Registers reg) {
        return static_cast<uint8_t>(reg) < static_cast<uint8_t>(Registers::AF) ? 1 : 2;
    }

    InstructionResult Emulator::handleInstruction(uint32_t instruction) {

        const auto inst8 = static_cast<uint8_t>(instruction & 0x000000FF);
        const auto code = getOpCodeUnprefixed(inst8);
        const auto d16 = static_cast<uint16_t>((instruction >> 8) & 0x0000FFFF);
        const auto d8 = static_cast<uint8_t>((instruction >> 8) & 0x000000FF);
        const auto r8 = static_cast<int8_t>(d8); // signed offset

        log_info("{}", code);

        switch (inst8) {
            case 0x00: // NOP
                log_info("NOP");
                return InstructionResult{ 1, 4 };
            case 0x0E: // LD C, d8
                writeReg8(Registers::C, d8); 
                return InstructionResult{ 2, 8 };
            case 0x20: // JR NZ, r8
                log_info("JR NZ, {:x}", d8);
                if (!getFlag(Flag::ZERO)) {
                    //m_regPC += d8;
                    return InstructionResult{int8_t(2 + r8), 12 };
                }
                else {
                    return InstructionResult{ 2, 8 };
                }

            case 0x21: // LDD HL, d16
                log_info("LD HL, {:x}", d16);
                m_regHL = d16;
                return InstructionResult{ 3, 12 };
            case 0x31: // LD SP, d16
                log_info("LD SP, {:x}", d16);
                m_regSP = d16;
                return InstructionResult{ 3, 12 };
            case 0x32: // LDD (HL), A
                log_info("LDD (HL), A");
                write8(m_regHL, readReg8(Registers::A));
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
                log_error("UNKNOWN OPCODE: {:x}", inst8);
                EZ_FAIL();
        }
    }

    bool Emulator::getFlag(Flag flag) const {
        return ((0x1 << static_cast<uint8_t>(flag)) & lowByte(m_regAF)) != 0x0;
    }

    void Emulator::setFlag(Flag flag) {
        const uint8_t newVal = lowByte(m_regAF) | (0x1 << (static_cast<uint8_t>(flag)));
        setLowByte(m_regAF, newVal);
    }

    void Emulator::clearFlag(Flag flag) {
        const auto newVal = lowByte(m_regAF) & (~(0x1 << static_cast<uint8_t>(flag)));
        setLowByte(m_regAF, newVal);
    }

    uint8_t Emulator::readReg8(Registers reg) const {
        EZ_ASSERT(getRegisterSizeBytes(reg) == 1);
        switch (reg) {
            case Registers::A: return highByte(m_regAF);
            case Registers::F: return lowByte(m_regAF);
            case Registers::B: return highByte(m_regBC);
            case Registers::C: return lowByte(m_regBC);
            case Registers::D: return highByte(m_regDE);
            case Registers::E: return lowByte(m_regDE);
            case Registers::H: return highByte(m_regHL);
            case Registers::L: return lowByte(m_regHL);
            default: EZ_FAIL();
        }
    }

    void Emulator::writeReg8(Registers reg, uint8_t val) {
        EZ_ASSERT(getRegisterSizeBytes(reg) == 1);
        switch (reg) {
            case Registers::A: setHighByte(m_regAF, val); break;
            case Registers::F: setLowByte(m_regAF, val); break;
            case Registers::B: setHighByte(m_regBC, val); break;
            case Registers::C: setLowByte(m_regBC, val); break;
            case Registers::D: setHighByte(m_regDE, val); break;
            case Registers::E: setLowByte(m_regDE, val); break;
            case Registers::H: setHighByte(m_regHL, val); break;
            case Registers::L: setLowByte(m_regHL, val); break;
            default: EZ_FAIL();
        }
    }

    uint16_t Emulator::readReg16(Registers reg) const {
        EZ_ASSERT(getRegisterSizeBytes(reg) == 2);
        switch (reg) {
            case Registers::AF: return m_regAF;
            case Registers::BC: return m_regBC;
            case Registers::DE: return m_regDE;
            case Registers::HL: return m_regHL;
            default: EZ_FAIL();
        }
    }

    void Emulator::writeReg16(Registers reg, uint16_t val) {
        EZ_ASSERT(getRegisterSizeBytes(reg) == 2);
        switch (reg) {
            case Registers::AF: m_regAF = val; return;
            case Registers::BC: m_regBC = val; return;
            case Registers::DE: m_regDE = val; return;
            case Registers::HL: m_regHL = val; return;
            default: EZ_FAIL();
        }
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

    AddrInfo Emulator::getAddressInfo(uint16_t addr) const{

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


    uint32_t Emulator::read32(uint16_t addr) const {
        const auto addrInfo = getAddressInfo(addr);
        switch (addrInfo.m_bank) {
            case MemoryBank::ROM_0:
                return m_cart.read32(addr);
            case MemoryBank::WRAM_0:
                return *reinterpret_cast<const uint32_t*>(m_ram.data() + addr - addrInfo.m_baseAddr);
            case MemoryBank::VRAM:
                return *reinterpret_cast<const uint32_t*>(m_vram.data() + addr - addrInfo.m_baseAddr);
            default:
                EZ_FAIL();
                break;
        }
    }
}