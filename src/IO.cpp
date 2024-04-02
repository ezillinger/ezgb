#include "IO.h"

namespace ez {

uint8_t* IO::getMemPtrRW(uint16_t address) {
    const auto offset = address - IO_BASE_ADDR;
    EZ_ENSURE(size_t(offset) < IO_BYTES);
    return reinterpret_cast<uint8_t*>(&m_reg) + offset;
}

uint8_t IO::getMem8(uint16_t address) const { return const_cast<IO*>(this)->getMem8RW(address); }

const uint8_t* IO::getMemPtr(uint16_t address) const {
    return const_cast<IO*>(this)->getMemPtrRW(address);
}

} // namespace ez