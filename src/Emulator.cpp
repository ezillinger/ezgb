#include "Emulator.h"

namespace ez {

Emulator::Emulator(Cart& cart) : m_cart(cart) {}

uint8_t& Emulator::accessReg(RegAccess ra) {
    switch (ra) {
        case RegAccess::B:       return m_reg.b;
        case RegAccess::C:       return m_reg.c;
        case RegAccess::D:       return m_reg.d;
        case RegAccess::E:       return m_reg.e;
        case RegAccess::H:       return m_reg.h;
        case RegAccess::L:       return m_reg.l;
        case RegAccess::HL_ADDR: return *getMemPtrRW(m_reg.hl);
        case RegAccess::A:       return m_reg.a;
        default:                 EZ_FAIL();
    }
}

void Emulator::handleInstructionXOR8(OpCode oc) {
    uint8_t regValue = 0;
    switch (oc) {
        case OpCode::XOR_A_B: regValue = m_reg.b; break;
        case OpCode::XOR_A_C: regValue = m_reg.c; break;
        case OpCode::XOR_A_D: regValue = m_reg.d; break;
        case OpCode::XOR_A_E: regValue = m_reg.e; break;
        case OpCode::XOR_A_H: regValue = m_reg.h; break;
        case OpCode::XOR_A_L: regValue = m_reg.l; break;
        case OpCode::XOR_A_A: regValue = m_reg.a; break;
        default:              EZ_FAIL();
    }
    auto aVal = m_reg.a;
    aVal ^= regValue;
    m_reg.a = aVal;
    m_reg.f = 0;
    if (aVal == 0) {
        setFlag(Flag::ZERO);
    }
}

InstructionResult Emulator::handleInstructionCB(uint32_t instruction) {

    const auto opByte = static_cast<uint8_t>(instruction & 0x000000FF);
    // const auto u16 = static_cast<uint16_t>((instruction >> 8) & 0x0000FFFF);
    // const auto u8 = static_cast<uint8_t>((instruction >> 8) & 0x000000FF);
    // const auto i8 = static_cast<int8_t>(u8); // signed offset

    const auto top2Bits = (opByte & 0b11000000) >> 6;
    const auto bitIndex = (opByte & 0b00111000) >> 3;
    const auto regIndex = (opByte & 0b00000111);

    const auto opCode = OpCode{opByte};
    const auto info = getOpCodeInfoPrefixed(opByte);

    auto regAccess = RegAccess{regIndex};

    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    log_info("{}", info);

    switch (top2Bits) {
        case 0b01: // BIT
        {
            const bool isSet = accessReg(regAccess) & (0b1 << bitIndex);
            setFlag(Flag::ZERO, !isSet);
            setFlag(Flag::HALF_CARRY);
            clearFlag(Flag::SUBTRACT);
            break;
        }
        case 0b10: // RES
            EZ_FAIL();
            break;
        case 0b11: // SET
            EZ_FAIL();
            break;
        case 0b00:
            switch (opCode) {
                default: EZ_FAIL(); break;
            };
            break;
        default: log_error("Something terrible happened"); EZ_FAIL();
    }

    const auto cycles = branched ? info.m_cycles : info.m_cyclesIfBranch;
    const auto newPC = jumpAddr ? *jumpAddr : checked_cast<uint16_t>(m_reg.pc + info.m_size);
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
        case OpCode::LD_C_u8: m_reg.c = u8; break;
        case OpCode::JR_NZ_i8:
            if (!getFlag(Flag::ZERO)) {
                jumpAddr = m_reg.pc + info.m_size + i8;
                branched = true;
            } else {
                branched = false;
            }
            break;

        case OpCode::LD_HL_u16: m_reg.hl = u16; break;
        case OpCode::LD_SP_u16: m_reg.sp = u16; break;

        case OpCode::LD__HLminus__A: [[fallthrough]];
        case OpCode::LD__HL__A:
            getMem8RW(m_reg.hl) = m_reg.a;
            --m_reg.hl;
            break;
        case OpCode::LD__HLplus__A:
            getMem8RW(m_reg.hl) = m_reg.a;
            ++m_reg.hl;
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
        default:             log_error("UNKNOWN OPCODE: {:x}", opByte); EZ_FAIL();
    }

    const auto cycles = branched ? info.m_cycles : info.m_cyclesIfBranch;
    const auto newPC = jumpAddr ? *jumpAddr : checked_cast<uint16_t>(m_reg.pc + info.m_size);
    return InstructionResult{
        newPC,
        cycles,
    };
}

bool Emulator::getFlag(Flag flag) const { return ((0x1 << static_cast<uint8_t>(flag)) & lowByte(m_reg.af)) != 0x0; }

void Emulator::setFlag(Flag flag) {
    const uint8_t newVal = lowByte(m_reg.af) | (0x1 << (static_cast<uint8_t>(flag)));
    setLowByte(m_reg.af, newVal);
}

void Emulator::setFlag(Flag flag, bool value) { return value ? setFlag(flag) : clearFlag(flag); }

void Emulator::clearFlag(Flag flag) {
    const auto newVal = lowByte(m_reg.af) & (~(0x1 << static_cast<uint8_t>(flag)));
    setLowByte(m_reg.af, newVal);
}

bool Emulator::tick() {

    if (m_cyclesToWait == 0) {
        const auto instruction = *reinterpret_cast<const uint32_t*>(getMemPtr(m_reg.pc));
        auto result = InstructionResult{};
        if (m_prefix) {
            m_prefix = false;
            result = handleInstructionCB(instruction);
        } else {
            result = handleInstruction(instruction);
        }
        m_reg.pc = result.m_newPC;
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

uint8_t* Emulator::getMemPtrRW(uint16_t addr) {
    const auto addrInfo = getAddressInfo(addr);
    switch (addrInfo.m_bank) {
        case MemoryBank::ROM_0:  EZ_FAIL("Can't write to ROM!");
        case MemoryBank::WRAM_0: return m_ram.data() + addr - addrInfo.m_baseAddr;
        case MemoryBank::VRAM:   return m_vram.data() + addr - addrInfo.m_baseAddr;
        default:                 EZ_FAIL(); break;
    }
}

const uint8_t* Emulator::getMemPtr(uint16_t addr) const {
    const auto addrInfo = getAddressInfo(addr);
    switch (addrInfo.m_bank) {
        case MemoryBank::ROM_0:  return m_cart.data(addr);
        case MemoryBank::WRAM_0: return m_ram.data() + addr - addrInfo.m_baseAddr;
        case MemoryBank::VRAM:   return m_vram.data() + addr - addrInfo.m_baseAddr;
        default:                 EZ_FAIL(); break;
    }
}

} // namespace ez