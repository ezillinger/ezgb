#include "Emulator.h"
#include "MiscOps.h"
#include "OpCodes.h"

namespace ez {

Emulator::Emulator(Cart& cart, EmuSettings settings) : m_cart(cart), m_settings(settings) {
    //const auto bootromPath = "./roms/bootix_dmg.bin";
    const auto bootromPath = "./roms/dmg_boot.bin";
    log_info("Loading bootrom: {}", bootromPath);
    auto fp = fopen(bootromPath, "rb");
    ez_assert(fp);
    ez_assert(BOOTROM_BYTES == fread(m_bootrom.data(), 1, BOOTROM_BYTES, fp));
    fclose(fp);

    if (m_settings.m_skipBootROM) {
        m_reg.a = 0x01;
        m_reg.f = 0xB0;
        m_reg.b = 0x00;
        m_reg.c = 0x13;
        m_reg.d = 0x00;
        m_reg.e = 0xD8;
        m_reg.h = 0x01;
        m_reg.l = 0x4D;
        m_reg.sp = 0xfffe;
        m_reg.pc = 0x100;
        m_ioReg.m_bootromDisabled = true;
    }
}

uint16_t Emulator::readR16Mem(R16Mem r16) const {
    switch (r16) {
        case R16Mem::BC:  return m_reg.bc;
        case R16Mem::DE:  return m_reg.de;
        case R16Mem::HLI: return m_reg.hl;
        case R16Mem::HLD: return m_reg.hl;
        default:          fail("not implemented");
    }
}

bool Emulator::getCond(Cond c) const {
    switch (c) {
        case Cond::Z:  return getFlag(Flag::ZERO);
        case Cond::NZ: return !getFlag(Flag::ZERO);
        case Cond::C:  return getFlag(Flag::CARRY);
        case Cond::NC: return !getFlag(Flag::CARRY);
        default:       fail("not implemented");
    }
}

uint16_t Emulator::readR16(R16 r16) const {
    switch (r16) {
        case R16::BC: return m_reg.bc;
        case R16::DE: return m_reg.de;
        case R16::HL: return m_reg.hl;
        case R16::SP: return m_reg.sp;
        default:      fail("not implemented");
    }
}

void Emulator::writeR16(R16 r16, uint16_t data) {
    switch (r16) {
        case R16::BC: m_reg.bc = data; break;
        case R16::DE: m_reg.de = data; break;
        case R16::HL: m_reg.hl = data; break;
        case R16::SP: m_reg.sp = data; break;
        default:      fail("not implemented");
    }
}

uint16_t Emulator::readR16Stack(R16Stack r16) const {
    switch (r16) {
        case R16Stack::BC: return m_reg.bc;
        case R16Stack::DE: return m_reg.de;
        case R16Stack::HL: return m_reg.hl;
        case R16Stack::AF: return m_reg.af;
        default:           fail("not implemented");
    }
}

void Emulator::writeR16Stack(R16Stack r16, uint16_t data) {
    switch (r16) {
        case R16Stack::BC: m_reg.bc = data; break;
        case R16Stack::DE: m_reg.de = data; break;
        case R16Stack::HL: m_reg.hl = data; break;
        case R16Stack::AF: {
            m_reg.af = data;
            m_reg.f &= 0xF0; // bottom bits of flags are always 0
            break;
        }
        default: fail("not implemented");
    }
}

uint8_t Emulator::readR8(R8 r8) const {
    switch (r8) {
        case R8::B:       return m_reg.b;
        case R8::C:       return m_reg.c;
        case R8::D:       return m_reg.d;
        case R8::E:       return m_reg.e;
        case R8::H:       return m_reg.h;
        case R8::L:       return m_reg.l;
        case R8::HL_ADDR: return readAddr(m_reg.hl);
        case R8::A:       return m_reg.a;
        default:          fail("not implemented");
    }
}

void Emulator::writeR8(R8 r8, uint8_t data) {
    switch (r8) {
        case R8::B:       m_reg.b = data; break;
        case R8::C:       m_reg.c = data; break;
        case R8::D:       m_reg.d = data; break;
        case R8::E:       m_reg.e = data; break;
        case R8::H:       m_reg.h = data; break;
        case R8::L:       m_reg.l = data; break;
        case R8::HL_ADDR: writeAddr(m_reg.hl, data); break;
        case R8::A:       m_reg.a = data; break;
        default:          fail("not implemented");
    }
}

InstructionResult Emulator::handleInstructionCB(uint32_t pcData) {

    const auto opByte = static_cast<uint8_t>(pcData & 0x000000FF);

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
            const bool isSet = readR8(r8) & (0b1 << bitIndex);
            setFlag(Flag::ZERO, !isSet);
            setFlag(Flag::HALF_CARRY);
            clearFlag(Flag::NEGATIVE);
            break;
        }
        case 0b10: // RES
            writeR8(r8, readR8(r8) & ~(0b1 << bitIndex));
            break;
        case 0b11: // SET
            writeR8(r8, readR8(r8) | (0b1 << bitIndex));
            break;
        case 0b00: {
            switch (top5Bits) {
                case 0b00000: // rlc r8
                {
                    auto r8v = readR8(r8);
                    const auto topBit = r8v & 0b1000'0000;
                    const auto result = (r8v << 1) | (topBit ? 0b1 : 0b0);
                    writeR8(r8, result);
                    clearFlags();
                    setFlag(Flag::CARRY, topBit);
                    setFlag(Flag::ZERO, result == 0);
                    break;
                }
                case 0b00001: // rrc r8
                {
                    const auto r8v = readR8(r8);
                    const auto bottomBit = r8v & 0b1;
                    const auto result = (r8v >> 1) | (bottomBit ? 0b1000'0000 : 0b0);
                    writeR8(r8, result);
                    clearFlags();
                    setFlag(Flag::CARRY, bottomBit);
                    setFlag(Flag::ZERO, result == 0);
                    break;
                }
                case 0b00010: { // rl r8
                    auto r8val = readR8(r8);
                    const auto setCarry = bool(0b1000'0000 & r8val);
                    const auto lastBit = getFlag(Flag::CARRY) ? 0b1 : 0b0;
                    r8val = (r8val << 1) | lastBit;
                    writeR8(r8, r8val);
                    clearFlags();
                    setFlag(Flag::CARRY, setCarry);
                    setFlag(Flag::ZERO, r8val == 0);
                } break;
                case 0b00011: // rr r8
                {
                    auto r8val = readR8(r8);
                    const auto setCarry = bool(0b1 & r8val);
                    const auto firstBit = getFlag(Flag::CARRY) ? 0b1000'0000 : 0b0;
                    r8val = (r8val >> 1) | firstBit;
                    writeR8(r8, r8val);
                    clearFlags();
                    setFlag(Flag::CARRY, setCarry);
                    setFlag(Flag::ZERO, r8val == 0);
                    break;
                }
                case 0b00100: // sla r8
                {
                    const auto r8val = readR8(r8);
                    const uint8_t result = r8val << 1;
                    writeR8(r8, result);
                    clearFlags();
                    setFlag(Flag::ZERO, result == 0);
                    setFlag(Flag::CARRY, r8val & 0b1000'0000);
                    break;
                }
                case 0b00101: // sra r8
                {
                    const auto r8val = readR8(r8);
                    const auto signBit = 0b1000'0000 & r8val;
                    const auto lowBit = 0b1 & r8val;
                    const uint8_t result = (r8val >> 1) | signBit;
                    writeR8(r8, result);
                    clearFlags();
                    setFlag(Flag::ZERO, result == 0);
                    setFlag(Flag::CARRY, lowBit);
                    break;
                }
                case 0b00110: // swap r8
                {
                    const auto r8v_copy = readR8(r8);
                    auto r8v = readR8(r8);
                    r8v = r8v << 4 | (r8v_copy >> 4);
                    writeR8(r8, r8v);
                    clearFlags();
                    setFlag(Flag::ZERO, r8v == 0);
                    break;
                }
                case 0b00111: // srl r8
                {
                    const auto r8v = readR8(r8);
                    const auto result = r8v >> 1;
                    writeR8(r8, r8v >> 1);
                    clearFlags();
                    setFlag(Flag::ZERO, result == 0);
                    setFlag(Flag::CARRY, r8v & 0b1);
                    break;
                }
                default: fail("not implemented"); break;
            }
        } break;
        default: fail("should never get here");
    }
    if (branched) {
        assert(jumpAddr);
        if (m_settings.m_logEnable) {
            log_info("Took branch to {:#06x}", *jumpAddr);
        }
    }

    const auto cycles = branched ? *info.m_cyclesIfBranch : info.m_cycles;
    const auto newPC = jumpAddr ? *jumpAddr : checked_cast<uint16_t>(m_reg.pc + info.m_size);
    return InstructionResult{
        newPC,
        cycles,
    };
}

InstructionResult Emulator::handleInstructionBlock3(uint32_t pcData) {

    const auto oc = OpCode{checked_cast<uint8_t>(pcData & 0xFF)};
    ez_assert(((+oc & 0b11000000) >> 6) == 3);

    auto info = getOpCodeInfoUnprefixed(+oc);

    const auto u16 = static_cast<uint16_t>((pcData >> 8) & 0x0000FFFF);
    const auto u8 = static_cast<uint8_t>((pcData >> 8) & 0x000000FF);
    const auto i8 = static_cast<int8_t>((pcData >> 8) & 0x000000FF);
    const auto r16stack = checked_cast<R16Stack>((+oc & 0b00110000) >> 4);

    bool branched = false;
    auto jumpAddr = std::optional<uint16_t>{};

    const auto maybeDoJump = [&](bool condition) {
        if (condition) {
            jumpAddr = u16;
            branched = true;
        }
    };

    const auto maybeDoCall = [&](bool condition, bool setBranched) {
        if (condition) {
            // todo, verify timing - different values on different sources
            m_reg.sp -= 2;
            writeAddr16(m_reg.sp, m_reg.pc + info.m_size);
            jumpAddr = u16;
            if (setBranched) {
                branched = true;
            }
        }
    };

    const auto maybeDoRet = [&](bool condition, bool setBranched) {
        if (condition) {
            jumpAddr = readAddr16(m_reg.sp);
            m_reg.sp += 2;
            if (setBranched) {
                branched = true;
            }
        }
    };

    const auto last3bits = +oc & 0b111;
    const auto last4bits = +oc & 0b1111;
    if (last3bits == 0b111) { // rst tgt3
        const auto tgt3 = ((+oc & 0b00111000) >> 3) * 8;
        if (oc == OpCode::RST_08h) {
            assert(tgt3 == 0x08);
        }
        m_reg.sp -= 2;
        writeAddr16(m_reg.sp, m_reg.pc + info.m_size);
        jumpAddr = tgt3;
    } else if (last4bits == 0b0101) { // push r16stack
        m_reg.sp -= sizeof(uint16_t);
        writeAddr16(m_reg.sp, readR16Stack(r16stack));
    } else if (last4bits == 0b0001) { // pop r16stack
        writeR16Stack(r16stack, readAddr16(m_reg.sp));
        m_reg.sp += sizeof(uint16_t);
    } else {
        switch (oc) {
            case OpCode::PREFIX: m_prefix = true; break;
            // enable/disable interrupts after instruction after this one finishes
            case OpCode::EI: 
            if(!m_pendingInterruptEnableCycleCount){
                m_pendingInterruptEnableCycleCount = T_CYCLES_PER_M_CYCLE + 1; // hack so we get one instruction before interrupts enabled
            }
            break;
            case OpCode::DI:
                m_interruptMasterEnable = false;
                m_pendingInterruptEnableCycleCount = 0;
                break;

            case OpCode::LD__C__A:    writeAddr(0xFF00 + m_reg.c, m_reg.a); break;
            case OpCode::LD_A__a16_:  m_reg.a = readAddr(u16); break;
            case OpCode::LDH__a8__A:  writeAddr(0xFF00 + u8, m_reg.a); break;
            case OpCode::LD__a16__A:  writeAddr(u16, m_reg.a); break;
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
            case OpCode::JP_a16:     jumpAddr = u16; break;
            case OpCode::LDH_A__a8_: m_reg.a = readAddr(u8 + 0xFF00); break;
            case OpCode::AND_A_u8:
                m_reg.a &= u8;
                clearFlags();
                setFlag(Flag::ZERO, m_reg.a == 0);
                setFlag(Flag::HALF_CARRY);
                break;
            case OpCode::ADD_A_u8: {
                const uint8_t result = m_reg.a + u8;
                setFlag(Flag::ZERO, result == 0);
                clearFlag(Flag::NEGATIVE);
                setFlag(Flag::HALF_CARRY, getFlagHC_ADD(m_reg.a, u8));
                setFlag(Flag::CARRY, getFlagC_ADD(m_reg.a, u8));
                m_reg.a = result;
                break;
            }
            case OpCode::SUB_A_u8: {
                const uint8_t result = m_reg.a - u8;
                setFlag(Flag::ZERO, result == 0);
                setFlag(Flag::NEGATIVE, true);
                setFlag(Flag::HALF_CARRY, getFlagHC_SUB(m_reg.a, u8));
                setFlag(Flag::CARRY, getFlagC_SUB(m_reg.a, u8));
                m_reg.a = result;
                break;
            }
            case OpCode::XOR_A_u8: {
                m_reg.a ^= u8;
                clearFlags();
                setFlag(Flag::ZERO, m_reg.a == 0);
                break;
            }
            case OpCode::ADC_A_u8: {
                const auto carryBit = getFlag(Flag::CARRY) ? 0b1 : 0b0;
                const uint8_t result = m_reg.a + u8 + carryBit;
                clearFlag(Flag::NEGATIVE);
                setFlag(Flag::ZERO, result == 0);
                // wtf? https://gbdev.gg8.se/wiki/articles/ADC
                setFlag(Flag::HALF_CARRY, getFlagHC_ADC(m_reg.a, u8, carryBit));
                setFlag(Flag::CARRY, getFlagC_ADC(m_reg.a, u8, carryBit));
                m_reg.a = result;
                break;
            }
            case OpCode::JP_HL:   jumpAddr = m_reg.hl; break;
            case OpCode::OR_A_u8: {
                m_reg.a |= u8;
                clearFlags();
                setFlag(Flag::ZERO, m_reg.a == 0);
                break;
            }
            case OpCode::JP_NZ_a16:    maybeDoJump(!getFlag(Flag::ZERO)); break;
            case OpCode::JP_Z_a16:     maybeDoJump(getFlag(Flag::ZERO)); break;
            case OpCode::JP_C_a16:     maybeDoJump(getFlag(Flag::CARRY)); break;
            case OpCode::JP_NC_a16:    maybeDoJump(!getFlag(Flag::CARRY)); break;
            case OpCode::LD_HL_SPplus: {
                const uint16_t result = m_reg.sp + i8;
                clearFlags();
                // flags are set using unsigned byte, even though result is signed
                setFlag(Flag::HALF_CARRY, (m_reg.sp ^ u8 ^ result) & 0x10);
                setFlag(Flag::CARRY, int(0xFF & m_reg.sp) + u8 > 0xFF);
                m_reg.hl = result;
                break;
            }
            case OpCode::LD_SP_HL:  m_reg.sp = m_reg.hl; break;
            case OpCode::ADD_SP_i8: {
                const uint16_t result = m_reg.sp + i8;
                clearFlags();
                // flags are set using unsigned byte, even though result is signed
                setFlag(Flag::HALF_CARRY, (m_reg.sp ^ u8 ^ result) & 0x10);
                setFlag(Flag::CARRY, int(0xFF & m_reg.sp) + u8 > 0xFF);
                m_reg.sp = result;
                break;
            }
            case OpCode::SBC_A_u8: {
                const auto carryBit = getFlag(Flag::CARRY) ? 0b1 : 0;
                const uint8_t result = m_reg.a - u8 - carryBit;
                setFlag(Flag::NEGATIVE);
                setFlag(Flag::ZERO, result == 0);
                setFlag(Flag::HALF_CARRY, getFlagHC_SBC(m_reg.a, u8, carryBit));
                setFlag(Flag::CARRY, getFlagC_SBC(m_reg.a, u8, carryBit));
                m_reg.a = result;
                break;
            }
            case OpCode::LD_A__C_: {
                m_reg.a = readAddr(0xFF00 + m_reg.c);
                break;
            }
            case OpCode::RETI: {
                jumpAddr = readAddr16(m_reg.sp);
                m_reg.sp += 2;
                m_pendingInterruptEnableCycleCount = T_CYCLES_PER_M_CYCLE + 1; // hack so we get one instruction before interrupts enabled
                break;
            }
            default: fail("not implemented: {}", +oc);
        }
    }
    if (branched) {
        assert(jumpAddr);
        if (m_settings.m_logEnable) {
            log_info("Took branch to {:#06x}", *jumpAddr);
        }
    }

    const auto cycles = branched ? *info.m_cyclesIfBranch : info.m_cycles;
    const auto newPC = jumpAddr ? *jumpAddr : checked_cast<uint16_t>(m_reg.pc + info.m_size);
    return InstructionResult{
        newPC,
        cycles,
    };
}

InstructionResult Emulator::handleInstructionBlock2(uint32_t pcData) {

    const auto oc = OpCode{checked_cast<uint8_t>(pcData & 0xFF)};
    ez_assert(((+oc & 0b11000000) >> 6) == 2);

    const auto info = getOpCodeInfoUnprefixed(+oc);

    const auto top_5_bits = (+oc & 0b11111000) >> 3;
    const auto r8 = checked_cast<R8>(+oc & 0b111);

    switch (top_5_bits) {
        case 0b10000: // add a, r8
        {
            const auto r8val = readR8(r8);
            const uint8_t result = m_reg.a + r8val;
            setFlag(Flag::ZERO, result == 0);
            setFlag(Flag::NEGATIVE, false);
            setFlag(Flag::HALF_CARRY, getFlagHC_ADD(m_reg.a, r8val));
            setFlag(Flag::CARRY, getFlagC_ADD(m_reg.a, r8val));
            m_reg.a = result;
            break;
        }
        case 0b10001: // adc a, r8
        {
            const auto r8v = readR8(r8);
            const auto carryBit = getFlag(Flag::CARRY) ? 0b1 : 0b0;
            const uint8_t result = m_reg.a + r8v + carryBit;
            clearFlag(Flag::NEGATIVE);
            setFlag(Flag::ZERO, result == 0);
            setFlag(Flag::HALF_CARRY, getFlagHC_ADC(m_reg.a, r8v, carryBit));
            setFlag(Flag::CARRY, getFlagC_ADC(m_reg.a, r8v, carryBit));
            m_reg.a = result;
            break;
        }
        case 0b10010: // sub a, r8
        {
            const auto r8val = readR8(r8);
            const uint8_t result = m_reg.a - r8val;
            setFlag(Flag::ZERO, result == 0);
            setFlag(Flag::NEGATIVE, true);
            setFlag(Flag::HALF_CARRY, ((m_reg.a & 0xF) - (r8val & 0xF)) & 0x10);
            setFlag(Flag::CARRY, m_reg.a < r8val);
            m_reg.a = result;
            break;
        }
        case 0b10011: // sbc a, r8
        {
            const auto carryBit = getFlag(Flag::CARRY) ? 0b1 : 0;
            const auto r8v = readR8(r8);
            const uint8_t result = m_reg.a - r8v - carryBit;
            setFlag(Flag::NEGATIVE);
            setFlag(Flag::ZERO, result == 0);
            setFlag(Flag::HALF_CARRY, getFlagHC_SBC(m_reg.a, r8v, carryBit));
            setFlag(Flag::CARRY, getFlagC_SBC(m_reg.a, r8v, carryBit));
            m_reg.a = result;
            break;
        }
        case 0b10100: // and a, r8
            m_reg.a &= readR8(r8);
            clearFlags();
            setFlag(Flag::ZERO, m_reg.a == 0);
            setFlag(Flag::HALF_CARRY);
            break;
        case 0b10101: // xor a, r8
            m_reg.a ^= readR8(r8);
            clearFlags();
            setFlag(Flag::ZERO, m_reg.a == 0);
            break;
        case 0b10110: // or a, r8
            m_reg.a = m_reg.a | readR8(r8);
            clearFlags();
            setFlag(Flag::ZERO, m_reg.a == 0);
            break;
        case 0b10111: // cp a, r8
        {
            const auto r8val = readR8(r8);
            const uint8_t result = m_reg.a - r8val;
            setFlag(Flag::ZERO, result == 0);
            setFlag(Flag::NEGATIVE, true);
            setFlag(Flag::HALF_CARRY, ((m_reg.a & 0xF) - (r8val & 0xF)) & 0x10);
            setFlag(Flag::CARRY, m_reg.a < r8val);
            break;
        }
        default: fail("not implemented");
    }

    return InstructionResult{
        checked_cast<uint16_t>(m_reg.pc + info.m_size),
        info.m_cycles,
    };
}

InstructionResult Emulator::handleInstructionBlock1(uint32_t pcData) {

    const auto oc = OpCode{checked_cast<uint8_t>(pcData & 0xFF)};
    ez_assert(((+oc & 0b11000000) >> 6) == 0b01);

    const auto info = getOpCodeInfoUnprefixed(+oc);

    const auto srcR8 = checked_cast<R8>(+oc & 0b111);
    const auto dstR8 = checked_cast<R8>((+oc & 0b111000) >> 3);

    if (srcR8 == R8::HL_ADDR && dstR8 == R8::HL_ADDR) {
        // HALT
        m_haltMode = true;
        m_isInstructionAfterHaltMode = true;
        if(m_settings.m_logEnable){
            log_info("Enabling Halt Mode");
        }
    } else {
        writeR8(dstR8, readR8(srcR8));
    }
    return InstructionResult{
        checked_cast<uint16_t>(m_reg.pc + info.m_size),
        info.m_cycles,
    };
}

InstructionResult Emulator::handleInstructionBlock0(uint32_t pcData) {

    const auto oc = OpCode{checked_cast<uint8_t>(pcData & 0xFF)};
    ez_assert(((+oc & 0b11000000) >> 6) == 0);

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
        writeR16(r16, u16);
    } else if (is_ld_r16mem_a) {
        writeAddr(readR16Mem(r16mem), m_reg.a);
        if (r16mem == R16Mem::HLD) {
            --m_reg.hl;
        } else if (r16mem == R16Mem::HLI) {
            ++m_reg.hl;
        }
    } else if (is_ld_a_r16mem) {
        m_reg.a = readAddr(readR16Mem(r16mem));
        if (r16mem == R16Mem::HLD) {
            --m_reg.hl;
        } else if (r16mem == R16Mem::HLI) {
            ++m_reg.hl;
        }
    } else if (is_inc_r16) {
        writeR16(r16, readR16(r16) + 1);
    } else if (is_dec_r16) {
        writeR16(r16, readR16(r16) - 1);
    } else if (is_add_hl_r16) {
        const auto r16v = readR16(r16);
        const uint16_t result = m_reg.hl + r16v;
        clearFlag(Flag::NEGATIVE);
        setFlag(Flag::CARRY, (uint32_t(m_reg.hl) + r16v) > 0xFFFF);
        setFlag(Flag::HALF_CARRY, (m_reg.hl ^ r16v ^ result) & 0x1000);
        m_reg.hl = result;
    } else if (is_inc_r8) {
        const auto r8val = readR8(r8);
        const uint8_t result = r8val + 1;
        setFlag(Flag::ZERO, result == 0);
        setFlag(Flag::HALF_CARRY, getFlagHC_ADD(r8val, 1));
        clearFlag(Flag::NEGATIVE);
        writeR8(r8, result);
    } else if (is_dec_r8) {
        const auto r8val = readR8(r8);
        const uint8_t result = r8val - 1;
        setFlag(Flag::ZERO, result == 0);
        setFlag(Flag::HALF_CARRY, getFlagHC_SUB(r8val, 1));
        setFlag(Flag::NEGATIVE);
        writeR8(r8, result);
    } else if (is_ld_r8_u8) {
        writeR8(r8, u8);
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
            case OpCode::LD__a16__SP: writeAddr16(u16, m_reg.sp); break;
            case OpCode::RRA:         {
                const auto setCarry = bool(0b1 & m_reg.a);
                const auto firstBit = getFlag(Flag::CARRY) ? 0b1000'0000 : 0b0;
                m_reg.a = (m_reg.a >> 1) | firstBit;
                clearFlags();
                setFlag(Flag::CARRY, setCarry);
                break;
            }
            case OpCode::RLCA: {
                const auto topBit = m_reg.a & 0b1000'0000;
                m_reg.a = (m_reg.a << 1) | (topBit ? 0b1 : 0b0);
                clearFlags();
                setFlag(Flag::CARRY, topBit);
                break;
            }
            case OpCode::RRCA: {
                const auto bottomBit = m_reg.a & 0b1;
                m_reg.a = (m_reg.a >> 1) | (bottomBit ? 0b1000'0000 : 0b0);
                clearFlags();
                setFlag(Flag::CARRY, bottomBit);
                break;
            }
            case OpCode::STOP_u8: {
                if (readAddr(0xFF4D) & 0b1) {
                    // KEY1 register bit 0 set while stop doubles clock speed on GBC
                    log_warn("GBC Speed toggle ignored");
                } else {
                    m_stopMode = true;
                    m_ioReg.m_timerDivider = 0;
                }
                break;
            }
            case OpCode::DAA: {
                // i have no idea how this works
                // ripped from:
                // https://stackoverflow.com/questions/45227884/z80-daa-implementation-and-blarggs-test-rom-issues
                int tmp = m_reg.a;
                if (!(getFlag(Flag::NEGATIVE))) {
                    if ((getFlag(Flag::HALF_CARRY)) || (tmp & 0x0F) > 9)
                        tmp += 6;
                    if ((getFlag(Flag::CARRY)) || tmp > 0x9F)
                        tmp += 0x60;
                } else {
                    if (getFlag(Flag::HALF_CARRY)) {
                        tmp -= 6;
                        if (!(getFlag(Flag::CARRY)))
                            tmp &= 0xFF;
                    }
                    if (getFlag(Flag::CARRY)) {
                        tmp -= 0x60;
                    }
                }
                clearFlag(Flag::HALF_CARRY);
                clearFlag(Flag::ZERO);
                if (tmp & 0x100) {
                    setFlag(Flag::CARRY);
                }
                m_reg.a = tmp & 0xFF;
                setFlag(Flag::ZERO, m_reg.a == 0);

                break;
            }
            case OpCode::CPL: {
                m_reg.a = ~m_reg.a;
                setFlag(Flag::NEGATIVE);
                setFlag(Flag::HALF_CARRY);
                break;
            }
            case OpCode::SCF: {
                setFlag(Flag::CARRY);
                clearFlag(Flag::HALF_CARRY);
                clearFlag(Flag::NEGATIVE);
                break;
            }
            case OpCode::CCF: {
                setFlag(Flag::CARRY, !getFlag(Flag::CARRY));
                clearFlag(Flag::HALF_CARRY);
                clearFlag(Flag::NEGATIVE);
                break;
            }
            default: fail("not implemented"); break;
        }
    }

    if (branched) {
        assert(jumpAddr);
        if (m_settings.m_logEnable) {
            log_info("Took branch to {:#06x}", *jumpAddr);
        }
    }
    const auto cycles = branched ? *info.m_cyclesIfBranch : info.m_cycles;
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

    const auto block = (+oc & 0b11000000) >> 6;
    switch (block) {
        case 0b00: return handleInstructionBlock0(pcData); break;
        case 0b01: return handleInstructionBlock1(pcData); break;
        case 0b10: return handleInstructionBlock2(pcData); break;
        case 0b11: return handleInstructionBlock3(pcData); break;
        default:   fail("should never get here"); break;
    }
}

bool Emulator::getFlag(Flag flag) const { return ((0x1 << +flag) & m_reg.f) != 0x0; }

void Emulator::setFlag(Flag flag) { m_reg.f |= (0x1 << +flag); }

void Emulator::setFlag(Flag flag, bool value) { return value ? setFlag(flag) : clearFlag(flag); }

void Emulator::clearFlag(Flag flag) { m_reg.f &= ~(0x1 << +flag); }

void Emulator::clearFlags() { m_reg.f = 0x00; }

void Emulator::tick() {

    if (m_stopMode) {
        if (m_settings.m_autoUnStop) {
            log_warn("Auto-unstopping");
            m_stopMode = false;
        }
        // todo, recover from stop mode!
        return;
    }

    bool handledInstructionOrInterrupt = false;

    m_oamDmaCyclesRemaining = std::max(0, m_oamDmaCyclesRemaining - 1);

    tickTimers();

    

    // prefix instructions are atomic
    if (m_cyclesToWait == 0 && !m_prefix) {
        tickInterrupts();
        handledInstructionOrInterrupt = m_cyclesToWait != 0;
    }

    if(m_haltMode && m_cyclesToWait == 0){
        m_cyclesToWait = T_CYCLES_PER_M_CYCLE;
    }

    if (m_cyclesToWait == 0) {
        const auto word0 = readAddr16(m_reg.pc);
        const auto word1 = readAddr16(m_reg.pc + 2);
        const auto pcData = (uint32_t(word1) << 16) | (uint32_t(word0));
        auto result = InstructionResult{0, 1};
        maybe_log_registers();
        if (m_prefix) {
            m_prefix = false;
            result = handleInstructionCB(pcData);
        } else {
            result = handleInstruction(pcData);
        }
        if(!m_haltBugTriggered){
            m_reg.pc = result.m_newPC;
        }
        else{
            log_warn("Halt bug triggered, skipping PC increment");
        }
        m_haltBugTriggered = false;
        m_cyclesToWait = result.m_cycles;
        assert(m_cyclesToWait > 0);
        handledInstructionOrInterrupt = true;
    }

    m_cyclesToWait = std::max(m_cyclesToWait - 1, 0);
    
    if (m_pendingInterruptEnableCycleCount > 0) {
        --m_pendingInterruptEnableCycleCount;
        if (m_pendingInterruptEnableCycleCount == 0) {
            //ez_assert(m_sysclk % 4 == 0);
            m_interruptMasterEnable = true;
        }
    }

    if (handledInstructionOrInterrupt) {
        assert((m_cyclesToWait + 1) % T_CYCLES_PER_M_CYCLE == 0);
        assert(m_cycleCounter % T_CYCLES_PER_M_CYCLE == 0);
    }

    m_ppu.tick();

    ++m_cycleCounter;

    return;
}

void Emulator::tickTimers() {

    auto sysclkBefore = m_sysclk;
    ++m_sysclk;

    m_ioReg.m_timerDivider = m_sysclk >> 8;

    if (m_pendingTimaOverflowCycles > 0) {
        --m_pendingTimaOverflowCycles;
        if(m_pendingTimaOverflowCycles == 0){
            //ez_assert((m_sysclk) % 4 == 0);
            m_ioReg.m_if.timer = true;
            m_ioReg.m_tima = m_ioReg.m_tma;
        }
    }
    
    if (is_tima_increment(sysclkBefore, m_sysclk, m_ioReg.m_tac, m_ioReg.m_tac)) {
        ++m_ioReg.m_tima;
        if (m_ioReg.m_tima == 0) {
            m_pendingTimaOverflowCycles = T_CYCLES_PER_M_CYCLE + 1;
        }
    }

    
}

void Emulator::tickInterrupts() {

    

    for (auto i = 0; i < +Interrupts::NUM_INTERRUPTS; ++i) {
        const uint8_t bitFlag = 0b1 << i;
        if (m_ioReg.m_ie.data & bitFlag && m_ioReg.m_if.data & bitFlag) {
            // todo, implement halt bug
            m_haltMode = false;
            if (m_interruptMasterEnable) {
                if (m_settings.m_logEnable) {
                    log_info("Calling ISR {}", i);
                }
                m_ioReg.m_if.data &= ~bitFlag;
                m_interruptMasterEnable = false;
                m_reg.sp -= 2;
                writeAddr16(m_reg.sp, m_reg.pc);
                ez_assert(m_cyclesToWait == 0);
                m_cyclesToWait = 20; // 5 m-cycles
                switch (i) {
                    case +Interrupts::VBLANK: m_reg.pc = 0x40; break;
                    case +Interrupts::JOYPAD: m_reg.pc = 0x60; break;
                    case +Interrupts::SERIAL: m_reg.pc = 0x58; break;
                    case +Interrupts::LCD:    m_reg.pc = 0x48; break;
                    case +Interrupts::TIMER:  m_reg.pc = 0x50; break;
                    default:                  fail("Should never get here");
                }
                // only one interrupt serviced per tick
                break;
            }
            else{
                if(m_isInstructionAfterHaltMode){
                    m_haltBugTriggered = true;
                    log_warn("Halt bug triggered!");
                }
            }
        }
    }
    m_isInstructionAfterHaltMode = false;
}

AddrInfo Emulator::getAddressInfo(uint16_t addr) const {
    if (Cart::ROM_RANGE.containsExclusive(addr)) {
        return {MemoryBank::ROM, 0};
    } else if (Cart::RAM_RANGE.containsExclusive(addr)) {
        return {MemoryBank::EXT_RAM, Cart::RAM_RANGE.m_min};
    } else if (WRAM0_ADDR_RANGE.containsExclusive(addr)) {
        return {MemoryBank::WRAM_0, WRAM0_ADDR_RANGE.m_min};
    } else if (WRAM1_ADDR_RANGE.containsExclusive(addr)) {
        return {MemoryBank::WRAM_1, WRAM0_ADDR_RANGE.m_min}; // not a typo
    } else if (PPU::VRAM_ADDR_RANGE.containsExclusive(addr)) {
        return {MemoryBank::VRAM, PPU::VRAM_ADDR_RANGE.m_min};
    } else if (PPU::OAM_ADDR_RANGE.containsExclusive(addr)) {
        return {MemoryBank::OAM, PPU::OAM_ADDR_RANGE.m_min};
    } else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return {MemoryBank::OAM, 0xFEA0};
    } else if (addr >= 0xFEA0 && addr <= 0xFEFF) {
        return {MemoryBank::NOT_USEABLE, 0xFEA0};
    } else if (IO_ADDR_RANGE.containsExclusive(addr)) {
        return {MemoryBank::IO, IO_ADDR_RANGE.m_min};
    } else if (MIRROR_ADDR_RANGE.containsExclusive(addr)) {
        return {MemoryBank::MIRROR, MIRROR_ADDR_RANGE.m_min};
    } else {
        fail("not implemented");
    }
}

void Emulator::writeAddr(uint16_t addr, uint8_t data) {
    if (addr == +IOAddr::TAC) {
        log_warn("TAC set to {}", data);
    }
    if (addr == +IOAddr::TIMA) {
        log_warn( "TIMA set to {}", data);
    }
    m_lastWrittenAddr = addr;
    const auto addrInfo = getAddressInfo(addr);
    switch (addrInfo.m_bank) {
        case MemoryBank::ROM:         m_cart.writeAddr(addr, data); break;
        case MemoryBank::EXT_RAM:     m_cart.writeAddr(addr, data); break;
        case MemoryBank::WRAM_0:      [[fallthrough]];
        case MemoryBank::WRAM_1:      m_ram[addr - addrInfo.m_baseAddr] = data; break;
        case MemoryBank::MIRROR:      m_ram[(WRAM1_ADDR_RANGE.m_min - WRAM0_ADDR_RANGE.m_min) + (addr - addrInfo.m_baseAddr)] = data; break;
        case MemoryBank::VRAM:        m_ppu.writeAddr(addr, data); break;
        case MemoryBank::OAM:         m_ppu.writeAddr(addr, data); break;
        case MemoryBank::IO:          writeIO(addr, data); break;
        case MemoryBank::NOT_USEABLE: log_warn("Write to unusable zone: {}", addr); break;
        default:                      fail("not implemented"); break;
    }
}

void Emulator::writeAddr16(uint16_t addr, uint16_t data) {
    m_lastWrittenAddr = addr;
    writeAddr(addr, uint8_t(data & 0x00FF));
    writeAddr(addr + 1, uint8_t(data >> 8));
}

uint8_t Emulator::readAddr(uint16_t addr) const {
    const auto addrInfo = getAddressInfo(addr);
    switch (addrInfo.m_bank) {
        case MemoryBank::ROM:
            if (addr < BOOTROM_BYTES && !m_ioReg.m_bootromDisabled) {
                return m_bootrom[addr];
            }
            return m_cart.readAddr(addr);
        case MemoryBank::EXT_RAM:     return m_cart.readAddr(addr);
        case MemoryBank::WRAM_0:      [[fallthrough]];
        case MemoryBank::WRAM_1:      return m_ram[addr - addrInfo.m_baseAddr];
        case MemoryBank::MIRROR:      return m_ram[(WRAM1_ADDR_RANGE.m_min - WRAM0_ADDR_RANGE.m_min) + (addr - addrInfo.m_baseAddr)];
        case MemoryBank::VRAM:        return m_ppu.readAddr(addr);
        case MemoryBank::OAM:         return m_ppu.readAddr(addr);
        case MemoryBank::IO:          return readIO(addr);
        case MemoryBank::NOT_USEABLE: log_warn("Read from unusable zone: {}", addr); return 0xFF;
        default:                      fail("not implemented"); break;
    }
}
void Emulator::writeIO(uint16_t addr, uint8_t val) {

    EZ_ENSURE(IO_ADDR_RANGE.containsExclusive(addr));
    switch (addr) {
        case +IOAddr::SB: m_serialOutput.push_back(std::bit_cast<uint8_t>(val));
            m_ioReg.m_serialData = val;
            return;
        case +IOAddr::DIV:
        {
            m_ioReg.m_timerDivider = 0;
            if (is_tima_increment(m_sysclk, 0, m_ioReg.m_tac, m_ioReg.m_tac)) {
                log_warn("TIMA incr from div write");
                ++m_ioReg.m_tima;
                if (m_ioReg.m_tima == 0) {
                    m_pendingTimaOverflowCycles = T_CYCLES_PER_M_CYCLE;
                }
            }
            m_sysclk = 0;
            return;
        }
        case +IOAddr::TAC:
            if (is_tima_increment(m_sysclk, m_sysclk, m_ioReg.m_tac, val)) {
                log_warn("TIMA incr from TAC write");
                ++m_ioReg.m_tima;
                if (m_ioReg.m_tima == 0) {
                    m_pendingTimaOverflowCycles = T_CYCLES_PER_M_CYCLE;
                }
            }
            m_ioReg.m_tac = val;
            return;
        case +IOAddr::TIMA: {
            m_ioReg.m_tima = val;
            // todo, verify overwriting value with modulo if same cycle
            if(m_pendingTimaOverflowCycles == T_CYCLES_PER_M_CYCLE){
                log_warn("TIMA written to on overflow tick");
                m_pendingTimaOverflowCycles = 0;
            }
            return;
        }
        case +IOAddr::DMA: {
            m_oamDmaCyclesRemaining = 160 * T_CYCLES_PER_M_CYCLE;
            const uint16_t srcAddr = uint16_t(val) << 8;
            const uint16_t dstAddr = PPU::OAM_ADDR_RANGE.m_min;
            // todo, dont' do the copy instantaneously
            for (auto offset = 0; offset < PPU::OAM_ADDR_RANGE.width(); ++offset) {
                writeAddr(dstAddr + offset, readAddr(srcAddr + offset));
            }
            m_ioReg.m_lcd.m_dma = addr;
            break;
        }
        case +IOAddr::LY: {
            log_error("Write to LCD Y!");
            break;
        }
        case +IOAddr::STAT: {
            // ppu mode and ly==lyc are read only
            m_ioReg.m_lcd.m_status.m_data = (val & ~0b111) | (m_ioReg.m_lcd.m_status.m_data & 0b111);
            break;
        }
        case +IOAddr::LCDC: {
            m_ioReg.m_lcd.m_control.m_data = val;
            if(!m_ioReg.m_lcd.m_control.m_ppuEnable){
                m_ppu.reset();
            }
            break;
        }
        default: {
            const auto offset = addr - IO_ADDR_RANGE.m_min;
            reinterpret_cast<uint8_t*>(&m_ioReg)[offset] = val;
        }
        break;
    }
    
}

uint16_t Emulator::readAddr16(uint16_t addr) const { 
    const auto byte0 = readAddr(addr); 
    const auto byte1 = readAddr(addr + 1);

    return uint16_t(byte1) << 8 | uint16_t(byte0);
}

void Emulator::maybe_log_opcode(const OpCodeInfo& info) const {
    if (m_settings.m_logEnable) {
        log_info("{}", info);
    }
}

void Emulator::maybe_log_registers() const {
    if (m_settings.m_logEnable) {
        log_info("A {:#04x} B {:#04x} C {:#04x} D {:#04x} E {:#04x} F {:#04x} H {:#04x} L {:#04x}",
                 m_reg.a, m_reg.b, m_reg.c, m_reg.d, m_reg.e, m_reg.f, m_reg.h, m_reg.l);

        log_info("AF {:#06x} BC {:#06x} DE {:#06x} HL {:#06x} PC {:#06x} SP {:#06x} ", m_reg.af,
                 m_reg.bc, m_reg.de, m_reg.hl, m_reg.pc, m_reg.sp);

        log_info("Flags: Z {} N {} H {} C {}", getFlag(Flag::ZERO), getFlag(Flag::NEGATIVE),
                 getFlag(Flag::HALF_CARRY), getFlag(Flag::CARRY));
    }
}

const uint8_t* Emulator::getIOMemPtr(uint16_t addr) const {

    EZ_ENSURE(IO_ADDR_RANGE.containsExclusive(addr));
    // todo, check which registers are allowed to be written by CPU
    const auto offset = addr - IO_ADDR_RANGE.m_min;
    return reinterpret_cast<const uint8_t*>(&m_ioReg) + offset;
}



uint8_t Emulator::readIO(uint16_t addr) const {
    EZ_ENSURE(IO_ADDR_RANGE.containsExclusive(addr));
    const auto offset = addr - IO_ADDR_RANGE.m_min;
    return *(reinterpret_cast<const uint8_t*>(&m_ioReg) + offset);
}

} // namespace ez