#include "IO.h"
#include "MiscOps.h"

namespace ez {

void IO::setInterruptFlag(Interrupts f) { m_reg.m_if.data |= (0x1 << +f); }

void IO::tick() { tickTimers(); }

void IO::tickTimers() {

    auto sysclkBefore = m_sysclk;
    m_sysclk++; // 4 t cycles per m cycle

    m_reg.m_timerDivider = m_sysclk >> 8;

    if (m_reg.m_tac & 0b100) {
        static bool firstTime = true;
        if (firstTime) {
            log_warn("FIRST TIME TIMER ON");
            firstTime = false;
        }
        if (m_pendingTimaOverflow) {
            setInterruptFlag(Interrupts::TIMER);
            m_reg.m_tima = m_reg.m_tma;
            m_pendingTimaOverflow = false;
        }
        if (is_tima_increment(sysclkBefore, m_sysclk, m_reg.m_tac, m_reg.m_tac)) {
            ++m_reg.m_tima;
            if (m_reg.m_tima == 0) {
                m_pendingTimaOverflow = true;
            }
        }
    }
}

uint8_t* IO::getMemPtrRW(uint16_t addr) {

    EZ_ENSURE(ADDRESS_RANGE.containsExclusive(addr));
    // todo, check which registers are allowed to be written by CPU
    const auto offset = addr - ADDRESS_RANGE.m_min;
    return reinterpret_cast<uint8_t*>(&m_reg) + offset;
}

void IO::writeAddr(uint16_t addr, uint8_t val) {

    EZ_ENSURE(ADDRESS_RANGE.containsExclusive(addr));
    switch (addr) {
        case +IOAddr::SB: m_serialOutput.push_back(std::bit_cast<uint8_t>(val));
            m_reg.m_serialData = val;
            return;
        case +IOAddr::DIV:
            // todo, do we only reset top byte?
            m_sysclk = 0; 
            m_reg.m_timerDivider = 0;
            if (is_tima_increment(m_sysclk, 0, m_reg.m_tac, m_reg.m_tac)) {
                ++m_reg.m_tima;
                if (m_reg.m_tima == 0) {
                    m_pendingTimaOverflow = true;
                }
            }
            return;
        case +IOAddr::TAC:
            if (is_tima_increment(m_sysclk, m_sysclk, m_reg.m_tac, val)) {
                ++m_reg.m_tima;
                if (m_reg.m_tima == 0) {
                    m_pendingTimaOverflow = true;
                }
            }
            m_reg.m_tac = val;
            return;
        case +IOAddr::TIMA: {
            m_reg.m_tima = val;
            m_pendingTimaOverflow = false;
            return;
        }
        default: break;
    }
    const auto offset = addr - ADDRESS_RANGE.m_min;
    // todo, check which registers are allowed to be written by CPU
    reinterpret_cast<uint8_t*>(&m_reg)[offset] = val;
}

void IO::writeAddr16(uint16_t addr, uint16_t val) {
    EZ_ASSERT(HRAM_RANGE.containsExclusive(addr));
    *reinterpret_cast<uint16_t*>(m_reg.m_hram + (addr - HRAM_RANGE.m_min)) = val;
}

uint8_t IO::readAddr(uint16_t addr) const {
    EZ_ENSURE(ADDRESS_RANGE.containsExclusive(addr));
    const auto offset = addr - ADDRESS_RANGE.m_min;
    return *(reinterpret_cast<const uint8_t*>(&m_reg) + offset);
}

uint16_t IO::readAddr16(uint16_t addr) const {
    EZ_ASSERT(HRAM_RANGE.containsExclusive(addr));
    return *reinterpret_cast<const uint16_t*>(m_reg.m_hram + (addr - HRAM_RANGE.m_min));
}

} // namespace ez