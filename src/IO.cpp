#include "IO.h"

namespace ez {

uint8_t* IO::getMemPtrRW(uint16_t address) {

    // todo, check which registers are allowed to be written by CPU
    const auto offset = address - IO_BASE_ADDR;
    EZ_ENSURE(size_t(offset) < IO_BYTES);
    return reinterpret_cast<uint8_t*>(&m_reg) + offset;
}


void IO::writeAddr(uint16_t address, uint8_t val){

    switch (address)
    {
    case 0xFF01:
        m_serialOutput.push_back(std::bit_cast<uint8_t>(val));
        log_info("Wrote to serial console: {}", m_serialOutput);
        break;
    default:
        break;
    }
    const auto offset = address - IO_BASE_ADDR;
    EZ_ENSURE(size_t(offset) < IO_BYTES);
    
    // todo, check which registers are allowed to be written by CPU
    reinterpret_cast<uint8_t*>(&m_reg)[offset] = val;
}

uint8_t IO::readAddr(uint16_t address) const { 
    const auto offset = address - IO_BASE_ADDR;
    EZ_ENSURE(size_t(offset) < IO_BYTES);
    return *reinterpret_cast<const uint8_t*>(&m_reg) + offset;
}

} // namespace ez