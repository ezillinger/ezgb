#pragma once
#include "Base.h"

namespace ez {

enum class CartType : uint8_t {
    ROM_ONLY = 0x00,
    MBC1 = 0x01,
    MBC1_RAM = 0x02,
    MBC1_RAM_BATTERY = 0x03,
    MBC2 = 0x05,
    MBC2_BATTERY = 0x06,
    ROM_RAM_1 = 0x08,
    ROM_RAM_BATTERY_1 = 0x09,
    MMM01 = 0x0B,
    MMM01_RAM = 0x0C,
    MMM01_RAM_BATTERY = 0x0D,
    MBC3_TIMER_BATTERY = 0x0F,
    MBC3_TIMER_RAM_BATTERY_2 = 0x10,
    MBC3 = 0x11,
    MBC3_RAM_2 = 0x12,
    MBC3_RAM_BATTERY_2 = 0x13,
    MBC5 = 0x19,
    MBC5_RAM = 0x1A,
    MBC5_RAM_BATTERY = 0x1B,
    MBC5_RUMBLE = 0x1C,
    MBC5_RUMBLE_RAM = 0x1D,
    MBC5_RUMBLE_RAM_BATTERY = 0x1E,
    MBC6 = 0x20,
    MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
    POCKET_CAMERA = 0xFC,
    BANDAI_TAMA5 = 0xFD,
    HuC3 = 0xFE,
    HuC1_RAM_BATTERY = 0xFF,
};

class Cart {
  public:
    Cart(const fs::path& path);
    Cart(const uint8_t* data, size_t len);

    const uint8_t* data(uint16_t byteOffset) const;
    uint8_t readAddr(uint16_t addr) const { return m_data[addr]; }
    uint16_t readAddr16(uint16_t addr) const {
        return *reinterpret_cast<const uint16_t*>(m_data.data() + addr);
    };

    void writeAddr(uint16_t addr, uint8_t val);
    void writeAddr16(uint16_t addr, uint16_t val);

    bool isReadOnlyAddr(uint16_t addr);

  private:
    CartType m_cartType = CartType::ROM_ONLY;
    size_t m_sizeBytes = 0ull;
    std::vector<uint8_t> m_data;
};
} // namespace ez
