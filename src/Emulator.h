#pragma once
#include "Base.h"
#include "Cart.h"
#include "IO.h"
#include "OpCodes.h"
#include "PPU.h"

namespace ez {

class Tester;

enum class R8 { B, C, D, E, H, L, HL_ADDR, A };
enum class R16 { BC, DE, HL, SP };
enum class R16Mem { BC, DE, HLI, HLD };
enum class R16Stack { BC, DE, HL, AF };
enum class Cond { NZ, Z, NC, C };

struct EmuSettings {
    bool m_runAsFastAsPossible = true;
    bool m_logEnable = false;
    bool m_autoUnStop = false;
    bool m_skipBootROM = true;
    int32_t m_breakOnPC = -1;
    int32_t m_breakOnOpCode = -1;
    int32_t m_breakOnOpCodePrefixed = -1;
};

enum class MemoryBank {
    ROM_0,
    ROM_NN,
    VRAM,
    WRAM_0,
    WRAM_1,
    MIRROR,
    SPRITES,
    FUCK_OFF,
    IO,
    HRAM,
    IE,
    INVALID
};

struct AddrInfo {
    MemoryBank m_bank = MemoryBank::INVALID;
    uint16_t m_baseAddr = 0;
};

struct InstructionResult {
    InstructionResult() = default;
    InstructionResult(uint16_t newPC, int cycles) : m_newPC(newPC), m_cycles(cycles) {
        EZ_ASSERT(cycles > 0);
    }
    uint16_t m_newPC = 0;
    int m_cycles = 0;
};

struct Reg {
    uint16_t pc = 0;
    uint16_t sp = 0;
    union {
        struct {
            uint8_t f;
            uint8_t a;
        };
        uint16_t af = 0;
    };
    union {
        struct {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc = 0;
    };
    union {
        struct {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de = 0;
    };
    union {
        struct {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl = 0;
    };
    uint8_t ie = 0;
};

enum class Flag { ZERO = 7, NEGATIVE = 6, HALF_CARRY = 5, CARRY = 4 };

class Emulator {
  public:
    friend class Tester;
    friend class Gui;

    Emulator(Cart& cart, EmuSettings = {});
    bool tick();

  private:
    AddrInfo getAddressInfo(uint16_t address) const;

    void writeAddr(uint16_t address, uint8_t val);
    void writeAddr16(uint16_t address, uint16_t val);

    uint8_t readAddr(uint16_t address) const;
    uint16_t readAddr16(uint16_t address) const;

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
    void clearFlags();

    void writeR8(R8 ra, uint8_t data);
    uint8_t readR8(R8 ra) const;

    uint16_t readR16(R16 ra) const;
    void writeR16(R16 ra, uint16_t data);

    uint16_t readR16Mem(R16Mem ra) const;
    void writeR16Mem(R16Mem ra, uint16_t data);

    uint16_t readR16Stack(R16Stack ra) const;
    void writeR16Stack(R16Stack ra, uint16_t data);

    bool getCond(Cond c) const;

    Reg m_reg{};
    IO m_io{};
    PPU m_ppu{m_io.getLCDRegisters()};

    uint16_t m_cyclesToWait = 0;

    static constexpr auto MASTER_CLOCK_PERIOD = 239ns;

    Stopwatch m_tickStopwatch{};

    bool m_shouldExit = false;
    bool m_stopMode = false;
    bool m_prefix = false; // was last instruction CB prefix
    bool m_interruptsEnabled = false;
    int m_pendingInterruptsEnableCount = 0;  // enable interrupts when reaches 0
    int m_pendingInterruptsDisableCount = 0; // ^ disable

    void maybe_log_registers() const;
    void maybe_log_opcode(const OpCodeInfo& oc) const;

    static constexpr size_t HRAM_BYTES = 128;
    static constexpr size_t RAM_BYTES = 8 * 1024;
    static constexpr size_t BOOTROM_BYTES = 256;

    std::vector<uint8_t> m_bootrom = std::vector<uint8_t>(BOOTROM_BYTES, 0u);
    std::vector<uint8_t> m_ram = std::vector<uint8_t>(RAM_BYTES, 0u);
    std::vector<uint8_t> m_hram = std::vector<uint8_t>(HRAM_BYTES, 0u);

    Cart& m_cart;
    EmuSettings m_settings{};
};
} // namespace ez