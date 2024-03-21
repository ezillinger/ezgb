#include "Emulator.h"

namespace ez {

Emulator::Emulator(Cart& cart) : m_cart(cart) {}

uint16_t& Emulator::getR16MemRW(R16Mem r16) {
    switch (r16) {
        case R16Mem::BC:  return m_reg.bc;
        case R16Mem::DE:  return m_reg.de;
        case R16Mem::HLI: return m_reg.hl;
        case R16Mem::HLD: return m_reg.hl;
        default:          EZ_FAIL();
    }
}

uint16_t& Emulator::getR16RW(R16 r16) {
    switch (r16) {
        case R16::BC: return m_reg.bc;
        case R16::DE: return m_reg.de;
        case R16::HL: return m_reg.hl;
        case R16::SP: return m_reg.sp;
        default:      EZ_FAIL();
    }
}

uint8_t Emulator::getR8(R8 r8) {
    switch (r8) {
        case R8::B:       return m_reg.b;
        case R8::C:       return m_reg.c;
        case R8::D:       return m_reg.d;
        case R8::E:       return m_reg.e;
        case R8::H:       return m_reg.h;
        case R8::L:       return m_reg.l;
        case R8::HL_ADDR: return *getMemPtr(m_reg.hl);
        case R8::A:       return m_reg.a;
        default:          EZ_FAIL();
    }
}

uint8_t& Emulator::getR8RW(R8 r8) {
    switch (r8) {
        case R8::B:       return m_reg.b;
        case R8::C:       return m_reg.c;
        case R8::D:       return m_reg.d;
        case R8::E:       return m_reg.e;
        case R8::H:       return m_reg.h;
        case R8::L:       return m_reg.l;
        case R8::HL_ADDR: return *getMemPtrRW(m_reg.hl);
        case R8::A:       return m_reg.a;
        default:          EZ_FAIL();
    }
}

InstructionResult Emulator::handleInstructionCB(uint32_t pcData) {

    const auto opByte = static_cast<uint8_t>(pcData & 0x000000FF);

    const auto top2Bits = (opByte & 0b11000000) >> 6;
    const auto bitIndex = (opByte & 0b00111000) >> 3;
    const auto regIndex = (opByte & 0b00000111);

    const auto opCode = OpCode{opByte};
    const auto info = getOpCodeInfoPrefixed(opByte);

    auto r8 = R8{regIndex};

    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    log_info("{}", info);

    switch (top2Bits) {
        case 0b01: // BIT r8 bitIndex
        {
            const bool isSet = getR8RW(r8) & (0b1 << bitIndex);
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
        case 0b00: // everything else
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

InstructionResult Emulator::handleInstructionBlock3(uint32_t pcData) {

    const auto oc = OpCode{checked_cast<uint8_t>(pcData & 0xFF)};
    EZ_ASSERT(((+oc & 0b11000000) >> 6) == 3);

    const auto info = getOpCodeInfoUnprefixed(+oc);
    log_info("{}", info);

    const auto u16 = static_cast<uint16_t>((pcData >> 8) & 0x0000FFFF);

    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    switch (oc) {
        case OpCode::PREFIX: m_prefix = true; break;
        case OpCode::EI:
            // enable interrupts after instruction after this one finishes
            m_pendingInterruptsEnableCount = 2;
            break;
        case OpCode::LD__C__A: getMem8RW(0xFF00 + m_reg.c) = m_reg.a; break;
        case OpCode::LD_A__a16_: m_reg.a = getMem8(u16); break;
        default: EZ_FAIL();
    }
    const auto cycles = branched ? info.m_cycles : info.m_cyclesIfBranch;
    const auto newPC = jumpAddr ? *jumpAddr : checked_cast<uint16_t>(m_reg.pc + info.m_size);
    return InstructionResult{
        newPC,
        cycles,
    };
}

InstructionResult Emulator::handleInstructionBlock2(uint32_t pcData) {

    const auto oc = OpCode{checked_cast<uint8_t>(pcData & 0xFF)};
    EZ_ASSERT(((+oc & 0b11000000) >> 6) == 2);

    const auto info = getOpCodeInfoUnprefixed(+oc);
    log_info("{}", info);

    const auto top_5_bits = (+oc & 0b11111000) >> 3;
    const auto r8 = checked_cast<R8>(+oc & 0b111);

    switch (top_5_bits) {
        case 0b10000: // add a, r8
            EZ_FAIL();
            break;
        case 0b10001: // adc a, r8
            EZ_FAIL();
            break;
        case 0b10010: // sub a, r8
            EZ_FAIL();
            break;
        case 0b10011: // sbc a, r8
            EZ_FAIL();
            break;
        case 0b10100: // and a, r8
            EZ_FAIL();
            break;
        case 0b10101: // xor a, r8
            m_reg.a ^= getR8RW(r8);
            m_reg.f = 0;
            if (m_reg.a == 0) {
                setFlag(Flag::ZERO);
            }
            break;
        case 0b10110: // or a, r8
            EZ_FAIL();
            break;
        case 0b10111: // cp a, r8
            EZ_FAIL();
            break;
        default: EZ_FAIL();
    }

    return InstructionResult{
        checked_cast<uint16_t>(m_reg.pc + info.m_size),
        info.m_cycles,
    };
}

InstructionResult Emulator::handleInstructionBlock1(uint32_t pcData) {

    const auto oc = OpCode{checked_cast<uint8_t>(pcData & 0xFF)};
    EZ_ASSERT(((+oc & 0b11000000) >> 6) == 1);

    const auto info = getOpCodeInfoUnprefixed(+oc);

    const auto srcR8 = checked_cast<R8>(+oc & 0b111);
    const auto dstR8 = checked_cast<R8>((+oc & 0b111000) >> 3);

    log_info("{}", info);

    if(srcR8 == R8::HL_ADDR && dstR8 == R8::HL_ADDR){
        // HALT
        EZ_FAIL();
    }
    else{
        getR8RW(dstR8) = getR8(srcR8);
    }
    return InstructionResult{
        checked_cast<uint16_t>(m_reg.pc + info.m_size),
        info.m_cycles,
    };
}

InstructionResult Emulator::handleInstructionBlock0(uint32_t pcData) {

    const auto oc = OpCode{checked_cast<uint8_t>(pcData & 0xFF)};
    EZ_ASSERT(((+oc & 0b11000000) >> 6) == 0);

    const auto info = getOpCodeInfoUnprefixed(+oc);
    log_info("{}", info);

    const auto last_4_bits = (+oc & 0b1111);
    const auto last_3_bits = (+oc & 0b111);

    const bool is_ld_r16_u16 = last_4_bits == 0b0001;
    const bool is_ld_r16mem_a = last_4_bits == 0b0010;
    const bool is_ld_a_r16mem = last_4_bits == 0b1010;
    const bool is_ld_u16mem_sp = last_4_bits == 0b1000;

    const bool is_inc_r16 = last_4_bits == 0b0011;
    const bool is_dec_r16 = last_4_bits == 0b1011;
    const bool is_add_hl_r16 = last_4_bits == 0b1001;

    const bool is_inc_r8 = last_3_bits == 0b100;
    const bool is_dec_r8 = last_3_bits == 0b101;

    const bool is_ld_r8_u8 = last_3_bits == 0b110;

    const auto u16 = static_cast<uint16_t>((pcData >> 8) & 0x0000FFFF);
    const auto u8 = static_cast<uint8_t>((pcData >> 8) & 0x000000FF);
    const auto r8 = R8{(+oc & 0b00111000) >> 3};
    const auto r16 = R16{(+oc & 0b00110000) >> 4};
    const auto r16mem = R16Mem{(+oc & 0b00110000) >> 4};
    // const auto i8 = static_cast<int8_t>(u8); // signed offset

    if (is_ld_r16_u16) {
        getR16RW(r16) = u16;
    } else if (is_ld_r16mem_a) {
        getR16MemRW(r16mem) = (0xFF + m_reg.a);
        if (r16mem == R16Mem::HLD) {
            --m_reg.hl;
        } else if (r16mem == R16Mem::HLI) {
            ++m_reg.hl;
        }
    } else if (is_ld_a_r16mem) {
        EZ_FAIL();
    } else if (is_ld_u16mem_sp) {
        EZ_FAIL();
    } else if (is_inc_r16) {
        auto& r16val = getR16RW(r16);
        ++r16val;
    } else if (is_dec_r16) {
        auto& r16val = getR16RW(r16);
        --r16val;
    } else if (is_add_hl_r16) {
        EZ_FAIL();
    } else if (is_inc_r8) {
        auto& r8val = getR8RW(r8);
        const auto wraparound = r8val == 0xFF;
        ++r8val;
        setFlag(Flag::ZERO, wraparound);
        setFlag(Flag::HALF_CARRY, wraparound);
        clearFlag(Flag::SUBTRACT);
    } else if (is_dec_r8) {
        auto& r8val = getR8RW(r8);
        const auto wraparound = r8val == 0x00;
        ++r8val;
        setFlag(Flag::ZERO, r8val == 0);
        setFlag(Flag::HALF_CARRY, wraparound);
        setFlag(Flag::SUBTRACT);
    } else if (is_ld_r8_u8) {
        getR8RW(r8) = u8;
    } else {
        switch (oc) {
            case OpCode::NOP: break;
            default:          EZ_FAIL(); break;
        }
    }
    return InstructionResult{
        checked_cast<uint16_t>(m_reg.pc + info.m_size),
        info.m_cycles,
    };
}

constexpr uint8_t Emulator::getRegisterSizeBytes(Registers reg) { return static_cast<uint8_t>(reg) < static_cast<uint8_t>(Registers::AF) ? 1 : 2; }

InstructionResult Emulator::handleInstruction(uint32_t pcData) {

    EZ_ENSURE(!m_prefix);

    const auto oc = OpCode{static_cast<uint8_t>(pcData & 0x000000FF)};

    const auto block = (+oc & 0b11000000) >> 6;
    switch (block) {
        case 0b00: return handleInstructionBlock0(pcData); break;
        case 0b01: return handleInstructionBlock1(pcData); break;
        case 0b10: return handleInstructionBlock2(pcData); break;
        case 0b11: return handleInstructionBlock3(pcData); break;
        default:   EZ_FAIL(); break;
    }
    /*

    const auto u16 = static_cast<uint16_t>((pcData >> 8) & 0x0000FFFF);
    const auto u8 = static_cast<uint8_t>((pcData >> 8) & 0x000000FF);
    const auto i8 = static_cast<int8_t>(u8); // signed offset


    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    log_info("{}", info);

    switch (oc) {
        case OpCode::LD__HL__u8:     handleInstructionLD_x_u8(oc);
        case OpCode::LD__C__A:       getMem8RW(0xFF00 + m_reg.c) = m_reg.a; break;
        case OpCode::LD_HL_u16:      m_reg.hl = u16; break;
        case OpCode::LD_SP_u16:      m_reg.sp = u16; break;
        case OpCode::JR_NZ_i8:
            if (!getFlag(Flag::ZERO)) {
                jumpAddr = m_reg.pc + info.m_size + i8;
                branched = true;
            } else {
                branched = false;
            }
            break;
    }
    */
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
        const auto pcData = *reinterpret_cast<const uint32_t*>(getMemPtr(m_reg.pc));
        auto result = InstructionResult{};
        if (m_prefix) {
            m_prefix = false;
            result = handleInstructionCB(pcData);
        } else {
            result = handleInstruction(pcData);
        }
        if (m_pendingInterruptsEnableCount > 0) {
            --m_pendingInterruptsEnableCount;
            if (m_pendingInterruptsEnableCount == 0) {
                m_interruptsEnabled = true;
            }
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
    } else if (addr >= 0xFF00 && addr < 0xFF80) {
        return {MemoryBank::IO, 0xFF00};
    } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        return {MemoryBank::HRAM, 0xFF80};
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
        case MemoryBank::IO:     return m_io.m_data.data() + addr - addrInfo.m_baseAddr;
        default:                 EZ_FAIL(); break;
    }
}

const uint8_t* Emulator::getMemPtr(uint16_t addr) const {
    const auto addrInfo = getAddressInfo(addr);
    switch (addrInfo.m_bank) {
        case MemoryBank::ROM_0:  return m_cart.data(addr);
        case MemoryBank::WRAM_0: return m_ram.data() + addr - addrInfo.m_baseAddr;
        case MemoryBank::VRAM:   return m_vram.data() + addr - addrInfo.m_baseAddr;
        case MemoryBank::IO:     return m_io.m_data.data() + addr - addrInfo.m_baseAddr;
        default:                 EZ_FAIL(); break;
    }
}

} // namespace ez