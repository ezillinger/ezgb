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

struct MBC1State {
    static constexpr size_t RAM_SIZE = 32 * 1024; // max possible size
    uint8_t m_ramEnable = 0;
    uint8_t m_romBankSelect = 0;
    uint8_t m_ramBankSelect = 0;
    uint8_t m_romRamModeSelect = 0;
    std::vector<uint8_t> m_ram = std::vector<uint8_t>(RAM_SIZE);

    bool isRamEnabled() const { return (m_ramEnable & 0x0A) == 0x0A; }
};

class Cart {
  public:
    friend class Tester;
    friend class Gui;

    Cart(const uint8_t* data, size_t len);
    static Cart loadFromDisk(const fs::path& path);

    uint8_t readAddr(uint16_t addr) const;

    void writeAddr(uint16_t addr, uint8_t val);

    static constexpr iRange ROM_RANGE = iRange{0x0000, 0x8000};
    static constexpr iRange RAM_RANGE = iRange{0xA000, 0xC000};

    static constexpr bool isValidAddr(uint16_t addr) {
        return ROM_RANGE.containsExclusive(addr) || RAM_RANGE.containsExclusive(addr);
    }

  private:
    const uint8_t* getROMPtr(uint16_t addr) const;
    // todo, add RAM bank switching
    uint8_t* getRAMPtr(uint16_t addr) {
        return m_mbc1State.m_ram.data() + (addr - RAM_RANGE.m_min);
    }
    const uint8_t* getRAMPtr(uint16_t addr) const {
        return m_mbc1State.m_ram.data() + (addr - RAM_RANGE.m_min);
    }

    static bool isMBC1Type(CartType type);

    CartType m_cartType = CartType::ROM_ONLY;
    size_t m_sizeBytes = 0ull;
    MBC1State m_mbc1State{};
    const std::vector<uint8_t> m_data;
};
} // namespace ez
