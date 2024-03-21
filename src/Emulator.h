#pragma once
#include "Cart.h"
#include "OpCodes.h"
#include "base.h"

namespace ez {

enum class R8 { B, C, D, E, H, L, HL_ADDR, A };
enum class R16 { BC, DE, HL, SP };
enum class R16Mem { BC, DE, HLI, HLD };

enum class MemoryBank { ROM_0, ROM_NN, VRAM, WRAM_0, WRAM_1, MIRROR, SPRITES, FUCK_OFF, IO, HRAM, IE, INVALID };

struct AddrInfo {
    MemoryBank m_bank = MemoryBank::INVALID;
    uint16_t m_baseAddr = 0;
};

struct InstructionResult {
    uint16_t m_newPC = 0;
    int m_cycles = 0;
};

struct Reg {
    uint16_t pc = 0;
    uint16_t sp = 0;
	union { struct { uint8_t a; uint8_t f; }; uint16_t af = 0; };
	union { struct { uint8_t b; uint8_t c; }; uint16_t bc = 0; };
	union { struct { uint8_t d; uint8_t e; }; uint16_t de = 0; };
	union { struct { uint8_t h; uint8_t l; }; uint16_t hl = 0; };
};

struct IO {
    std::array<uint8_t, 128> m_data;
};

enum class Registers {
    A = 0,
    F,
    B,
    C,
    D,
    E,
    H,
    L,
    AF,
    BC,
    DE,
    HL,
    NUM_REGISTERS
};

enum class Flag { ZERO = 7, SUBTRACT = 6, HALF_CARRY = 5, CARRY = 4 };

class Emulator {
  public:
    Emulator(Cart& cart);
    bool tick();

  private:
    AddrInfo getAddressInfo(uint16_t address) const;

    uint8_t* getMemPtrRW(uint16_t address);
    uint8_t& getMem8RW(uint16_t address) { return *getMemPtrRW(address); }

    const uint8_t* getMemPtr(uint16_t address) const;
    uint8_t getMem8(uint16_t address) const { return *getMemPtr(address); }

    static constexpr uint8_t getRegisterSizeBytes(Registers reg);

    InstructionResult handleInstruction(uint32_t instruction);
    InstructionResult handleInstructionCB(uint32_t instruction);

    InstructionResult handleInstructionBlock0(uint32_t pcdata);
    InstructionResult handleInstructionBlock1(uint32_t pcdata);
    InstructionResult handleInstructionBlock2(uint32_t pcdata);
    InstructionResult handleInstructionBlock3(uint32_t pcdata);

    bool getFlag(Flag flag) const;
    void setFlag(Flag flag);
    void setFlag(Flag flag, bool value);
    void clearFlag(Flag flag);


    uint8_t& getR8RW(R8 ra);
    uint8_t getR8(R8 ra);
    uint16_t& getR16RW(R16 ra);
    uint16_t& getR16MemRW(R16Mem ra);

    Reg m_reg{};
    IO m_io{};

    uint16_t m_cyclesToWait = 0;


    bool m_stop = false;
    bool m_prefix = false; // was last instruction CB prefix
    bool m_interruptsEnabled = false;
    int m_pendingInterruptsEnableCount = 0; // enable interrupts when reaches 0

    static constexpr size_t RAM_BYTES = 8 * 1024;

    std::vector<uint8_t> m_ram = std::vector<uint8_t>(RAM_BYTES, 0u);
    std::vector<uint8_t> m_vram = std::vector<uint8_t>(RAM_BYTES, 0u);

    Cart& m_cart;
};
} // namespace ez