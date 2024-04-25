#include "IO.h"

namespace ez {

void IO::setInterruptFlag(Interrupts f) { 
    m_reg.m_if.data |= (0x1 << +f); 
}

void IO::tick() {
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
        case +IOAddr::SB:
            m_serialOutput.push_back(std::bit_cast<uint8_t>(val));
            break;
        case +IOAddr::DIV: m_reg.m_timerDivider = 0; return; // any write resets div
        default:           break;
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