#pragma once
#include "Base.h"
#include "Cart.h"
#include "IO.h"
#include "OpCodes.h"
#include "PPU.h"

namespace ez {

class Tester;

enum class R8 {
    B,
    C,
    D,
    E,
    H,
    L,
    HL_ADDR,
    A
};
enum class R16 {
    BC,
    DE,
    HL,
    SP
};
enum class R16Mem {
    BC,
    DE,
    HLI,
    HLD
};
enum class R16Stack {
    BC,
    DE,
    HL,
    AF
};
enum class Cond {
    NZ,
    Z,
    NC,
    C
};

struct EmuSettings {
    bool m_logEnable = false;
    bool m_skipBootROM = false;
};

enum class MemoryBank {
    ROM,
    VRAM,
    EXT_RAM,
    WRAM_0,
    WRAM_1,
    MIRROR,
    OAM,
    IO,
    NOT_USEABLE,
    INVALID
};

struct AddrInfo {
    MemoryBank m_bank = MemoryBank::INVALID;
    int m_baseAddr = 0;
};

struct InstructionResult {
    InstructionResult() = default;
    InstructionResult(uint16_t newPC, int cycles) : m_newPC(newPC), m_cycles(cycles) {
        ez_assert(cycles > 0);
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
};

enum class Flag {
    ZERO = 7,
    NEGATIVE = 6,
    HALF_CARRY = 5,
    CARRY = 4
};

class Emulator {
  public:
    friend class Tester;
    friend class Gui;

    Emulator(Cart& cart, EmuSettings = {});
    void tick(const InputState& input); // one T-cycle tick
    bool executed_instr_this_cycle() { return m_executedInstructionThisCycle; }

    uint16_t get_pc() const { return m_reg.pc; }
    OpCodeInfo get_current_opcode() const {
        const auto opByte = read_addr(m_reg.pc);
        return m_prefix ? get_opcode_info_prefixed(opByte) : get_opcode_info(opByte);
    }

    // todo, make this less greasy
    int64_t get_cycle_counter() const { return m_cycleCounter; };
    int& get_last_written_addr() { return m_lastWrittenAddr; }
    bool want_breakpoint() {
        if (m_wantBreakpoint) {
            if (m_cyclesToWait == 1) {
                m_wantBreakpoint = false;
            }
            return true;
        }
        return false;
    }

    static constexpr auto MASTER_CLOCK_PERIOD = 239ns;
    static constexpr int T_CYCLES_PER_M_CYCLE = 4;

    std::span<const rgba8> get_display_framebuffer() const {
        return m_ppu.get_display_framebuffer();
    };
    std::span<const rgba8> get_window_dbg_framebuffer() {
        return m_ppu.get_window_dbg_framebuffer();
    };
    std::span<const rgba8> get_bg_dbg_framebuffer() { return m_ppu.get_bg_dbg_framebuffer(); };
    std::span<const rgba8> get_vram_dbg_framebuffer() { return m_ppu.get_vram_dbg_framebuffer(); };

    // for testing only
    const uint8_t* dbg_get_io_ptr(uint16_t addr) const;

  protected:
    void tick_countdowns();
    void tick_timers();
    void tick_interrupts();

    AddrInfo get_addr_info(uint16_t address) const;

    void write_addr(uint16_t address, uint8_t val);
    void write_addr_16(uint16_t address, uint16_t val);

    uint8_t read_addr(uint16_t address) const;
    uint16_t readAddr16(uint16_t address) const;

    void write_io(uint16_t addr, uint8_t val);
    uint8_t read_io(uint16_t addr) const;

    InstructionResult handle_instr(uint32_t instruction);
    InstructionResult handle_instr_prefixed(uint32_t instruction);

    InstructionResult handle_instr_b0(uint32_t pcdata);
    InstructionResult handle_instr_b1(uint32_t pcdata);
    InstructionResult handle_instr_b2(uint32_t pcdata);
    InstructionResult handle_instr_b3(uint32_t pcdata);

    bool get_flag(Flag flag) const;
    void set_flag(Flag flag);
    void set_flag(Flag flag, bool value);
    void clear_flag(Flag flag);
    void clear_all_flags();

    void write_R8(R8 ra, uint8_t data);
    uint8_t read_R8(R8 ra) const;

    uint16_t read_R16(R16 ra) const;
    void write_R16(R16 ra, uint16_t data);

    uint16_t read_R16Mem(R16Mem ra) const;
    void write_R16Mem(R16Mem ra, uint16_t data);

    uint16_t read_R16Stack(R16Stack ra) const;
    void write_R16Stack(R16Stack ra, uint16_t data);

    bool get_Cond(Cond c) const;

    Reg m_reg{};
    IORegisters m_ioReg;
    PPU m_ppu{m_ioReg};

    int64_t m_cycleCounter = 0;

    int m_lastWrittenAddr = -2;
    int m_cyclesToWait = 0;
    bool m_executedInstructionThisCycle = false;

    bool m_stopMode = false;

    bool m_haltMode = false;
    bool m_isInstructionAfterHaltMode = false;
    bool m_haltBugTriggered = false;

    bool m_prefix = false; // was last instruction CB prefix
    bool m_interruptMasterEnable = false;
    int m_pendingInterruptEnableCycleCount = 0; // enable interrupts when goes below 0

    bool m_wantBreakpoint = false;

    uint16_t m_sysclk = 0;               // t cycles
    int m_pendingTimaOverflowCycles = 0; // t cycles until TIMA overflow

    int m_oamDmaCyclesRemaining = 0;

    void maybe_log_registers() const;
    void maybe_log_opcode(const OpCodeInfo& oc) const;

    static constexpr size_t HRAM_BYTES = 128;
    static constexpr size_t RAM_BYTES = 8 * 1024;
    static constexpr size_t BOOTROM_BYTES = 256;

    std::vector<uint8_t> m_bootrom = std::vector<uint8_t>(BOOTROM_BYTES, 0u);
    std::vector<uint8_t> m_ram = std::vector<uint8_t>(RAM_BYTES, 0u);
    std::vector<uint8_t> m_hram = std::vector<uint8_t>(HRAM_BYTES, 0u);

    std::string m_serialOutput;

    Cart& m_cart;
    EmuSettings m_settings{};

    InputState m_inputState{};
};
} // namespace ez