#include "Emulator.h"

namespace ez {

Emulator::Emulator(Cart& cart) : m_cart(cart) {
    // const auto bootloaderPath = "./roms/bootix_dmg.bin";
    const auto bootloaderPath = "./roms/dmg_boot.bin";
    log_info("Loading bootrom: {}", bootloaderPath);
    auto fp = fopen(bootloaderPath, "rb");
    EZ_ASSERT(fp);
    EZ_ASSERT(BOOTROM_BYTES == fread(m_bootrom.data(), 1, BOOTROM_BYTES, fp));
    fclose(fp);
    m_tickStopwatch.reset();
}

uint16_t Emulator::getR16Mem(R16Mem r16) const {
    switch (r16) {
        case R16Mem::BC:  return m_reg.bc;
        case R16Mem::DE:  return m_reg.de;
        case R16Mem::HLI: return m_reg.hl;
        case R16Mem::HLD: return m_reg.hl;
        default:          EZ_FAIL("not implemented");
    }
}

bool Emulator::getCond(Cond c) const {
    switch (c) {
        case Cond::Z:  return getFlag(Flag::ZERO);
        case Cond::NZ: return !getFlag(Flag::ZERO);
        case Cond::C:  return getFlag(Flag::CARRY);
        case Cond::NC: return !getFlag(Flag::CARRY);
        default:       EZ_FAIL("not implemented");
    }
}

uint16_t& Emulator::getR16RW(R16 r16) {
    switch (r16) {
        case R16::BC: return m_reg.bc;
        case R16::DE: return m_reg.de;
        case R16::HL: return m_reg.hl;
        case R16::SP: return m_reg.sp;
        default:      EZ_FAIL("not implemented");
    }
}

uint16_t Emulator::getR16Stack(R16Stack r16) const {
    switch (r16) {
        case R16Stack::BC: return m_reg.bc;
        case R16Stack::DE: return m_reg.de;
        case R16Stack::HL: return m_reg.hl;
        case R16Stack::AF: return m_reg.af;
        default:           EZ_FAIL("not implemented");
    }
}

uint16_t& Emulator::getR16StackRW(R16Stack r16) {
    switch (r16) {
        case R16Stack::BC: return m_reg.bc;
        case R16Stack::DE: return m_reg.de;
        case R16Stack::HL: return m_reg.hl;
        case R16Stack::AF: return m_reg.af;
        default:           EZ_FAIL("not implemented");
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
        default:          EZ_FAIL("not implemented");
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
        default:          EZ_FAIL("not implemented");
    }
}

InstructionResult Emulator::handleInstructionCB(uint32_t pcData) {

    const auto opByte = static_cast<uint8_t>(pcData & 0x000000FF);

    if (opByte == m_settings.m_breakOnOpCodePrefixed) {
        EZ_DEBUG_BREAK();
    }

    const auto top2Bits = (opByte & 0b11000000) >> 6;
    const auto top5Bits = (opByte & 0b11111000) >> 3;
    const auto bitIndex = (opByte & 0b00111000) >> 3;

    const auto info = getOpCodeInfoPrefixed(opByte);

    auto r8 = R8{opByte & 0b00000111};

    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    maybe_log_opcode(info);

    switch (top2Bits) {
        case 0b01: // BIT r8 bitIndex
        {
            const bool isSet = getR8RW(r8) & (0b1 << bitIndex);
            setFlag(Flag::ZERO, !isSet);
            setFlag(Flag::HALF_CARRY);
            clearFlag(Flag::NEGATIVE);
            break;
        }
        case 0b10: // RES
            EZ_FAIL("not implemented");
            break;
        case 0b11: // SET
            EZ_FAIL("not implemented");
            break;
        case 0b00: {
            switch (top5Bits) {
                case 0b00000: // rlc r8
                    EZ_FAIL("not implemented");
                    break;
                case 0b00001: // rrc r8
                    EZ_FAIL("not implemented");
                    break;
                case 0b00010: { // rl r8
                    auto& r8val = getR8RW(r8);
                    const auto set_carry = bool(0b1000'0000 & r8val);
                    const auto last_bit = getFlag(Flag::CARRY) ? 0b1 : 0b0;
                    r8val = (r8val << 1) | last_bit;
                    setFlag(Flag::CARRY, set_carry);
                    setFlag(Flag::ZERO, r8val == 0);
                    clearFlag(Flag::HALF_CARRY);
                    clearFlag(Flag::NEGATIVE);
                } break;
                case 0b00011: // rr r8
                    EZ_FAIL("not implemented");
                    break;
                case 0b00100: // sla r8
                    EZ_FAIL("not implemented");
                    break;
                case 0b00101: // sra r8
                    EZ_FAIL("not implemented");
                    break;
                case 0b00110: // swap r8
                {
                    const auto r8v_copy = getR8(r8);
                    auto& r8v = getR8RW(r8);
                    r8v = r8v << 4 | (r8v_copy >> 4);
                    clearFlags();
                    setFlag(Flag::ZERO, r8v == 0);
                    break;
                }
                case 0b00111: // srl r8
                    EZ_FAIL("not implemented");
                    break;
                default: EZ_FAIL("not implemented"); break;
            }
        } break;
        default: EZ_FAIL("should never get here");
    }
    if (branched) {
        assert(jumpAddr);
        log_info("Took branch to {:#06x}", *jumpAddr);
    }

    const auto cycles = branched ? info.m_cyclesIfBranch : info.m_cycles;
    const auto newPC = jumpAddr ? *jumpAddr : checked_cast<uint16_t>(m_reg.pc + info.m_size);
    return InstructionResult{
        newPC,
        cycles,
    };
}

InstructionResult Emulator::handleInstructionBlock3(uint32_t pcData) {

    const auto oc = OpCode{checked_cast<uint8_t>(pcData & 0xFF)};
    EZ_ASSERT(((+oc & 0b11000000) >> 6) == 3);

    auto info = getOpCodeInfoUnprefixed(+oc);

    const auto u16 = static_cast<uint16_t>((pcData >> 8) & 0x0000FFFF);
    const auto u8 = static_cast<uint8_t>((pcData >> 8) & 0x000000FF);
    const auto r16stack = checked_cast<R16Stack>((+oc & 0b00110000) >> 4);

    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    const auto maybeDoCall = [&](bool condition, bool setBranched) {
        if (condition) {
            // todo, verify timing - different values on different sources
            m_reg.sp -= 2;
            getMem16RW(m_reg.sp) = (m_reg.pc + info.m_size);
            jumpAddr = u16;
            if (setBranched) {
                branched = true;
            }
        }
    };

    const auto maybeDoRet = [&](bool condition, bool setBranched) {
        if (condition) {
            jumpAddr = getMem16(m_reg.sp);
            m_reg.sp += 2;
            if (setBranched) {
                branched = true;
            }
        }
    };

    const auto last4bits = +oc & 0b1111;
    if (last4bits == 0b0101) { // push r16stack
        m_reg.sp -= sizeof(uint16_t);
        getMem16RW(m_reg.sp) = getR16Stack(r16stack);
    } else if (last4bits == 0b0001) { // pop r16stack
        getR16StackRW(r16stack) = getMem16(m_reg.sp);
        m_reg.sp += sizeof(uint16_t);
    } else {
        switch (oc) {
            case OpCode::PREFIX: m_prefix = true; break;
            // enable/disable interrupts after instruction after this one finishes
            case OpCode::EI: m_pendingInterruptsEnableCount = 2; break;
            case OpCode::DI: m_pendingInterruptsDisableCount = 2; break;

            case OpCode::LD__C__A:    getMem8RW(0xFF00 + m_reg.c) = m_reg.a; break;
            case OpCode::LD_A__a16_:  m_reg.a = getMem8(u16); break;
            case OpCode::LDH__a8__A:  getMem8RW(0xFF00 + u8) = m_reg.a; break;
            case OpCode::LD__a16__A:  getMem8RW(u16) = m_reg.a; break;
            case OpCode::CALL_a16:    maybeDoCall(true, false); break;
            case OpCode::CALL_C_a16:  maybeDoCall(getFlag(Flag::CARRY), true); break;
            case OpCode::CALL_NC_a16: maybeDoCall(!getFlag(Flag::CARRY), true); break;
            case OpCode::CALL_Z_a16:  maybeDoCall(getFlag(Flag::ZERO), true); break;
            case OpCode::CALL_NZ_a16: maybeDoCall(!getFlag(Flag::ZERO), true); break;
            case OpCode::RET:         maybeDoRet(true, false); break;
            case OpCode::RET_C:       maybeDoRet(getFlag(Flag::CARRY), true); break;
            case OpCode::RET_NC:      maybeDoRet(!getFlag(Flag::CARRY), true); break;
            case OpCode::RET_Z:       maybeDoRet(getFlag(Flag::ZERO), true); break;
            case OpCode::RET_NZ:      maybeDoRet(!getFlag(Flag::ZERO), true); break;
            case OpCode::CP_A_u8:     {
                const uint8_t result = m_reg.a - u8;
                setFlag(Flag::ZERO, result == 0);
                setFlag(Flag::CARRY, m_reg.a < u8);
                setFlag(Flag::HALF_CARRY, (m_reg.a & 0xF) < (u8 & 0xF));
                setFlag(Flag::NEGATIVE);

            } break;
            case OpCode::LDH_A__a8_: m_reg.a = getMem8(u8 + 0xFF00); break;
            default:                 EZ_FAIL("not implemented");
        }
    }
    if (branched) {
        assert(jumpAddr);
        log_info("Took branch to {:#06x}", *jumpAddr);
    }

    const auto cycles = branched ? info.m_cyclesIfBranch : info.m_cycles;
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

    const auto top_5_bits = (+oc & 0b11111000) >> 3;
    const auto r8 = checked_cast<R8>(+oc & 0b111);

    switch (top_5_bits) {
        case 0b10000: // add a, r8
        {
            const auto r8val = getR8(r8);
            const auto result = m_reg.a + r8val;
            setFlag(Flag::ZERO, result == 0);
            setFlag(Flag::NEGATIVE, false);
            setFlag(Flag::HALF_CARRY, (m_reg.a ^ r8val ^ result) & 0x10);
            setFlag(Flag::CARRY, int(m_reg.a) + r8val > 0xFF);
            m_reg.a = result;
            break;
        }
        case 0b10001: // adc a, r8
            EZ_FAIL("not implemented");
            break;
        case 0b10010: // sub a, r8
        {
            const auto r8val = getR8(r8);
            const auto result = m_reg.a - r8val;
            setFlag(Flag::ZERO, result == 0);
            setFlag(Flag::NEGATIVE, true);
            setFlag(Flag::HALF_CARRY, ((m_reg.a & 0xF) - (r8val & 0xF)) & 0x10);
            setFlag(Flag::CARRY, m_reg.a < r8val);
            m_reg.a = result;
            break;
        }
        case 0b10011: // sbc a, r8
            EZ_FAIL("not implemented");
            break;
        case 0b10100: // and a, r8
            EZ_FAIL("not implemented");
            break;
        case 0b10101: // xor a, r8
            m_reg.a ^= getR8RW(r8);
            m_reg.f = 0;
            if (m_reg.a == 0) {
                setFlag(Flag::ZERO);
            }
            break;
        case 0b10110: // or a, r8
            m_reg.a = m_reg.a | getR8(r8);
            clearFlags();
            setFlag(Flag::ZERO, m_reg.a == 0);
            break;
        case 0b10111: // cp a, r8
        {
            const auto r8val = getR8(r8);
            const auto result = m_reg.a - r8val;
            setFlag(Flag::ZERO, result == 0);
            setFlag(Flag::NEGATIVE, true);
            setFlag(Flag::HALF_CARRY, ((m_reg.a & 0xF) - (r8val & 0xF)) & 0x10);
            setFlag(Flag::CARRY, m_reg.a < r8val);
            break;
        }
        default: EZ_FAIL("not implemented");
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

    if (srcR8 == R8::HL_ADDR && dstR8 == R8::HL_ADDR) {
        // HALT
        EZ_FAIL("not implemented");
    } else {
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

    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    const auto last_4_bits = (+oc & 0b1111);
    const auto last_3_bits = (+oc & 0b111);

    const bool is_ld_r16_u16 = last_4_bits == 0b0001;
    const bool is_ld_r16mem_a = last_4_bits == 0b0010;
    const bool is_ld_a_r16mem = last_4_bits == 0b1010;

    const bool is_inc_r16 = last_4_bits == 0b0011;
    const bool is_dec_r16 = last_4_bits == 0b1011;
    const bool is_add_hl_r16 = last_4_bits == 0b1001;

    const bool is_inc_r8 = last_3_bits == 0b100;
    const bool is_dec_r8 = last_3_bits == 0b101;

    const bool is_ld_r8_u8 = last_3_bits == 0b110;
    const bool is_jr_cond_a8 = (+oc & 0b00100111) == 0b00100000;

    const auto u16 = static_cast<uint16_t>((pcData >> 8) & 0x0000FFFF);
    const auto u8 = static_cast<uint8_t>((pcData >> 8) & 0x000000FF);
    const auto i8 = static_cast<int8_t>(u8);

    const auto r8 = R8{(+oc & 0b00111000) >> 3};
    const auto r16 = R16{(+oc & 0b00110000) >> 4};
    const auto r16mem = R16Mem{(+oc & 0b00110000) >> 4};

    if (is_ld_r16_u16) {
        getR16RW(r16) = u16;
    } else if (is_ld_r16mem_a) {
        getMem8RW(getR16Mem(r16mem)) = m_reg.a;
        if (r16mem == R16Mem::HLD) {
            --m_reg.hl;
        } else if (r16mem == R16Mem::HLI) {
            ++m_reg.hl;
        }
    } else if (is_ld_a_r16mem) {
        m_reg.a = getMem8(getR16Mem(r16mem));
        if (r16mem == R16Mem::HLD) {
            --m_reg.hl;
        } else if (r16mem == R16Mem::HLI) {
            ++m_reg.hl;
        }
    } else if (is_inc_r16) {
        auto& r16val = getR16RW(r16);
        ++r16val;
    } else if (is_dec_r16) {
        auto& r16val = getR16RW(r16);
        --r16val;
    } else if (is_add_hl_r16) {
        EZ_FAIL("not implemented");
    } else if (is_inc_r8) {
        auto& r8val = getR8RW(r8);
        const auto wraparound = r8val == 0xFF;
        ++r8val;
        setFlag(Flag::ZERO, wraparound);
        setFlag(Flag::HALF_CARRY, wraparound);
        clearFlag(Flag::NEGATIVE);
    } else if (is_dec_r8) {
        auto& r8val = getR8RW(r8);
        const auto wraparound = r8val == 0x00;
        --r8val;
        setFlag(Flag::ZERO, r8val == 0);
        setFlag(Flag::HALF_CARRY, wraparound);
        setFlag(Flag::NEGATIVE);
    } else if (is_ld_r8_u8) {
        getR8RW(r8) = u8;
    } else if (is_jr_cond_a8 || oc == OpCode::JR_i8) {
        const auto cond = checked_cast<Cond>(((+oc & 0b11000) >> 3));
        if (oc == OpCode::JR_i8 || getCond(cond)) {
            branched = oc != OpCode::JR_i8;
            jumpAddr = checked_cast<uint16_t>(m_reg.pc + info.m_size + i8);
        }
    } else {
        switch (oc) {
            case OpCode::NOP: break;
            case OpCode::RLA: {
                const auto set_carry = bool(0b1000'0000 & m_reg.a);
                const auto last_bit = getFlag(Flag::CARRY) ? 0b1 : 0b0;
                m_reg.a = (m_reg.a << 1) | last_bit;
                clearFlags();
                setFlag(Flag::CARRY, set_carry);
            } break;
            case OpCode::LD__a16__SP: getMem16RW(u16) = m_reg.sp; break;
            default:                  EZ_FAIL("not implemented"); break;
        }
    }

    if (branched) {
        assert(jumpAddr);
        if (m_settings.m_logEnable) {
            log_info("Took branch to {:#06x}", *jumpAddr);
        }
    }
    const auto cycles = branched ? info.m_cyclesIfBranch : info.m_cycles;
    const auto newPC = jumpAddr.value_or(checked_cast<uint16_t>(m_reg.pc + info.m_size));
    return InstructionResult{
        newPC,
        cycles,
    };
}

InstructionResult Emulator::handleInstruction(uint32_t pcData) {

    EZ_ENSURE(!m_prefix);

    const auto oc = OpCode{static_cast<uint8_t>(pcData & 0x000000FF)};
    const auto info = getOpCodeInfoUnprefixed(+oc);
    maybe_log_opcode(info);

    if (+oc == m_settings.m_breakOnOpCode) {
        EZ_DEBUG_BREAK();
    }

    const auto block = (+oc & 0b11000000) >> 6;
    switch (block) {
        case 0b00: return handleInstructionBlock0(pcData); break;
        case 0b01: return handleInstructionBlock1(pcData); break;
        case 0b10: return handleInstructionBlock2(pcData); break;
        case 0b11: return handleInstructionBlock3(pcData); break;
        default:   EZ_FAIL("should never get here"); break;
    }
}

bool Emulator::getFlag(Flag flag) const { return ((0x1 << +flag) & m_reg.f) != 0x0; }

void Emulator::setFlag(Flag flag) { m_reg.f |= (0x1 << +flag); }

void Emulator::setFlag(Flag flag, bool value) { return value ? setFlag(flag) : clearFlag(flag); }

void Emulator::clearFlag(Flag flag) { m_reg.f &= ~(0x1 << +flag); }

void Emulator::clearFlags() { m_reg.f = 0x00; }

bool Emulator::tick() {

    if (m_reg.pc == m_settings.m_breakOnPC) {
        EZ_DEBUG_BREAK();
    }

    if (!m_settings.m_runAsFastAsPossible && !m_tickStopwatch.lapped(MASTER_CLOCK_PERIOD)) {
        return m_stop;
    }

    if (m_cyclesToWait == 0) {
        const auto pcData = *reinterpret_cast<const uint32_t*>(getMemPtr(m_reg.pc));
        auto result = InstructionResult{0, 1};
        maybe_log_registers();
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
        if (m_pendingInterruptsDisableCount > 0) {
            --m_pendingInterruptsDisableCount;
            if (m_pendingInterruptsDisableCount == 0) {
                m_interruptsEnabled = false;
            }
        }
        m_reg.pc = result.m_newPC;
        m_cyclesToWait = result.m_cycles;
        assert(m_cyclesToWait > 0);
    }
    --m_cyclesToWait;

    for (auto i = 0; i < 8; ++i) {
        m_ppu.tick();
    }

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
        EZ_FAIL("not implemented");
    }
}

uint8_t* Emulator::getMemPtrRW(uint16_t addr) {
    const auto addrInfo = getAddressInfo(addr);
    switch (addrInfo.m_bank) {
        case MemoryBank::ROM_0:  EZ_FAIL("Can't write to ROM!");
        case MemoryBank::WRAM_0: return m_ram.data() + addr - addrInfo.m_baseAddr;
        case MemoryBank::VRAM:   return m_ppu.getMemPtrRW(addr);
        case MemoryBank::IO:     return m_io.getMemPtrRW(addr);
        case MemoryBank::HRAM:   return m_hram.data() + addr - addrInfo.m_baseAddr;
        default:                 EZ_FAIL("not implemented"); break;
    }
}

const uint8_t* Emulator::getMemPtr(uint16_t addr) const {
    const auto addrInfo = getAddressInfo(addr);
    switch (addrInfo.m_bank) {
        case MemoryBank::ROM_0:
            if (addr < BOOTROM_BYTES && m_io.is_bootrom_mapped()) {
                return m_bootrom.data() + addr;
            }
            return m_cart.data(addr);
        case MemoryBank::WRAM_0: return m_ram.data() + addr - addrInfo.m_baseAddr;
        case MemoryBank::VRAM:   return m_ppu.getMemPtr(addr);
        case MemoryBank::IO:     return m_io.getMemPtr(addr);
        case MemoryBank::HRAM:   return m_hram.data() + addr - addrInfo.m_baseAddr;
        default:                 EZ_FAIL("not implemented"); break;
    }
}

void Emulator::maybe_log_opcode(const OpCodeInfo& info) const {
    if (m_settings.m_logEnable) {
        log_info("{}", info);
    }
}

void Emulator::maybe_log_registers() const {
    static Stopwatch sw{};
    if (m_settings.m_logEnable || sw.lapped(1s)) {
        log_info("A {:#04x} B {:#04x} C {:#04x} D {:#04x} E {:#04x} F {:#04x} H {:#04x} L {:#04x}",
                 m_reg.a, m_reg.b, m_reg.c, m_reg.d, m_reg.e, m_reg.f, m_reg.h, m_reg.l);

        log_info("AF {:#06x} BC {:#06x} DE {:#06x} HL {:#06x} PC {:#06x} SP {:#06x} ", m_reg.af,
                 m_reg.bc, m_reg.de, m_reg.hl, m_reg.pc, m_reg.sp);

        log_info("Flags: Z {} N {} H {} C {}", getFlag(Flag::ZERO), getFlag(Flag::NEGATIVE),
                 getFlag(Flag::HALF_CARRY), getFlag(Flag::CARRY));
    }
}

} // namespace ez