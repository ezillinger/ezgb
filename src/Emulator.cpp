#include "Emulator.h"

namespace ez {

Emulator::Emulator(Cart& cart) : m_cart(cart) {}

void Emulator::handleInstructionXOR8(OpCode instruction) {
    uint8_t regValue = 0;
    switch (instruction) {
    case OpCode::XOR_A_B: regValue = readReg8(Registers::B); break;
    case OpCode::XOR_A_C: regValue = readReg8(Registers::C); break;
    case OpCode::XOR_A_D: regValue = readReg8(Registers::D); break;
    case OpCode::XOR_A_E: regValue = readReg8(Registers::E); break;
    case OpCode::XOR_A_H: regValue = readReg8(Registers::H); break;
    case OpCode::XOR_A_L: regValue = readReg8(Registers::L); break;
    case OpCode::XOR_A_A: regValue = readReg8(Registers::A); break;
    default:            EZ_FAIL();
    }
    auto aVal = readReg8(Registers::A);
    aVal ^= regValue;
    writeReg8(Registers::A, aVal);
    writeReg8(Registers::F, 0);
    if (aVal == 0) {
        setFlag(Flag::ZERO);
    }
}

void Emulator::handleInstructionBIT(OpCode instruction) { 
    log_info("{}", +instruction); 
}

InstructionResult Emulator::handleInstructionCB(uint32_t instruction) {

    const auto opByte = static_cast<uint8_t>(instruction & 0x000000FF);
    // const auto u16 = static_cast<uint16_t>((instruction >> 8) & 0x0000FFFF);
    // const auto u8 = static_cast<uint8_t>((instruction >> 8) & 0x000000FF);
    // const auto i8 = static_cast<int8_t>(u8); // signed offset

    const auto opCode = OpCode{opByte};
    const auto info = getOpCodeInfoPrefixed(opByte);

    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    log_info("{}", info);

    switch (opCode) {
    default: log_error("Unknown OpCode: {}", +opCode); EZ_FAIL();
    }

    const auto cycles = branched ? info.m_cycles : info.m_cyclesIfBranch;
    const auto newPC = jumpAddr ? *jumpAddr : checked_cast<uint16_t>(m_regPC + info.m_size);
    return InstructionResult{
        newPC,
        cycles,
    };
}

constexpr uint8_t Emulator::getRegisterSizeBytes(Registers reg) { return static_cast<uint8_t>(reg) < static_cast<uint8_t>(Registers::AF) ? 1 : 2; }

InstructionResult Emulator::handleInstruction(uint32_t instruction) {

    EZ_ENSURE(!m_prefix);

    const auto opByte = static_cast<uint8_t>(instruction & 0x000000FF);
    const auto u16 = static_cast<uint16_t>((instruction >> 8) & 0x0000FFFF);
    const auto u8 = static_cast<uint8_t>((instruction >> 8) & 0x000000FF);
    const auto i8 = static_cast<int8_t>(u8); // signed offset

    const auto opCode = OpCode{opByte};
    const auto info = getOpCodeInfoUnprefixed(opByte);

    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    log_info("{}", info);

    switch (opCode) {
    case OpCode::NOP:     break;
    case OpCode::LD_C_u8: writeReg8(Registers::C, u8); break;
    case OpCode::JR_NZ_i8:
        if (!getFlag(Flag::ZERO)) {
            jumpAddr = m_regPC + info.m_size + i8;
            branched = true;
        } else {
            branched = false;
        }
        break;

    case OpCode::LD_HL_u16: m_regHL = u16; break;
    case OpCode::LD_SP_u16: m_regSP = u16; break;

    case OpCode::LD__HLminus__A: [[fallthrough]];
    case OpCode::LD__HL__A:
        write8(m_regHL, readReg8(Registers::A));
        --m_regHL;
        break;
    case OpCode::LD__HLplus__A:
        write8(m_regHL, readReg8(Registers::A));
        ++m_regHL;
        break;

    // 8 bit xors
    case OpCode::XOR_A_A: [[fallthrough]];
    case OpCode::XOR_A_B: [[fallthrough]];
    case OpCode::XOR_A_C: [[fallthrough]];
    case OpCode::XOR_A_D: [[fallthrough]];
    case OpCode::XOR_A_E: [[fallthrough]];
    case OpCode::XOR_A_H: [[fallthrough]];
    case OpCode::XOR_A_L: handleInstructionXOR8(opCode); break;

    case OpCode::PREFIX: m_prefix = true; break;
    default:                log_error("UNKNOWN OPCODE: {:x}", opByte); EZ_FAIL();
    }

    const auto cycles = branched ? info.m_cycles : info.m_cyclesIfBranch;
    const auto newPC = jumpAddr ? *jumpAddr : checked_cast<uint16_t>(m_regPC + info.m_size);
    return InstructionResult{
        newPC,
        cycles,
    };
}

bool Emulator::getFlag(Flag flag) const { return ((0x1 << static_cast<uint8_t>(flag)) & lowByte(m_regAF)) != 0x0; }

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
    default:           EZ_FAIL();
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
    default:           EZ_FAIL();
    }
}

uint16_t Emulator::readReg16(Registers reg) const {
    EZ_ASSERT(getRegisterSizeBytes(reg) == 2);
    switch (reg) {
    case Registers::AF: return m_regAF;
    case Registers::BC: return m_regBC;
    case Registers::DE: return m_regDE;
    case Registers::HL: return m_regHL;
    default:            EZ_FAIL();
    }
}

void Emulator::writeReg16(Registers reg, uint16_t val) {
    EZ_ASSERT(getRegisterSizeBytes(reg) == 2);
    switch (reg) {
    case Registers::AF: m_regAF = val; return;
    case Registers::BC: m_regBC = val; return;
    case Registers::DE: m_regDE = val; return;
    case Registers::HL: m_regHL = val; return;
    default:            EZ_FAIL();
    }
}

bool Emulator::tick() {

    if (m_cyclesToWait == 0) {
        auto instruction = read32(m_regPC);
        auto result = InstructionResult{};
        if (m_prefix) {
            m_prefix = false;
            result = handleInstructionCB(instruction);
        } else {
            result = handleInstruction(instruction);
        }
        m_regPC = result.m_newPC;
        m_cyclesToWait = result.m_cycles;
    }
    --m_cyclesToWait;

    return m_stop;
}

AddrInfo Emulator::getAddressInfo(uint16_t addr) const {

    if (addr < 0x3FFF) {
        return {MemoryBank::ROM_0, 0};
    } else if (addr >= 0xC000 && addr <= 0xCFFF) {
        return {MemoryBank::WRAM_0, 0xC000};
    } else if (addr >= 0x8000 && addr <= 0x9FFF) {
        return {MemoryBank::VRAM, 0x8000};
    } else {
        EZ_FAIL();
    }
}

void Emulator::write8(uint16_t addr, uint8_t value) {
    const auto addrInfo = getAddressInfo(addr);
    switch (addrInfo.m_bank) {
    case MemoryBank::ROM_0:  EZ_FAIL();
    case MemoryBank::WRAM_0: m_ram[addr - addrInfo.m_baseAddr] = value; break;
    case MemoryBank::VRAM:   m_vram[addr - addrInfo.m_baseAddr] = value; break;
    default:                 EZ_FAIL(); break;
    }
}

uint32_t Emulator::read32(uint16_t addr) const {
    const auto addrInfo = getAddressInfo(addr);
    switch (addrInfo.m_bank) {
    case MemoryBank::ROM_0:  return m_cart.read32(addr);
    case MemoryBank::WRAM_0: return *reinterpret_cast<const uint32_t*>(m_ram.data() + addr - addrInfo.m_baseAddr);
    case MemoryBank::VRAM:   return *reinterpret_cast<const uint32_t*>(m_vram.data() + addr - addrInfo.m_baseAddr);
    default:                 EZ_FAIL(); break;
    }
}

} // namespace ez