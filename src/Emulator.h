#pragma once
#include "base.h"
#include "Cart.h"

namespace ez
{
	struct Instruction {
		std::string m_mnemonic;
		uint8_t m_size = 0;
		uint8_t m_cycles = 0;

		std::optional<uint8_t> m_data8;
		std::optional<uint8_t> m_data16;
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
		int8_t m_size = 0;
		uint8_t m_cycles = 0;
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

	enum class Flag {
		ZERO = 7,
		SUBTRACT = 6,
		HALF_CARRY = 5,
		CARRY = 4
	};

	class Emulator
	{
	public:
		Emulator(Cart& cart);
		bool tick();

	private:
		AddrInfo getAddressInfo(uint16_t address) const;

		uint8_t read8(uint16_t address) const ;
		uint16_t read16(uint16_t address) const;
		uint32_t read32(uint16_t address) const;

		void write8(uint16_t address, uint8_t value);
		void write16(uint16_t address, uint16_t value);

		static constexpr uint8_t getRegisterSizeBytes(Registers reg);

		InstructionResult handleInstruction(uint32_t instruction);
		InstructionResult handleInstructionXOR8(uint8_t instruction);
		InstructionResult handleInstructionCB(uint8_t instruction);

		bool getFlag(Flag flag) const;
		void setFlag(Flag flag);
		void clearFlag(Flag flag);

		uint8_t readReg8(Registers reg) const;
		void writeReg8(Registers reg, uint8_t val);

		uint16_t readReg16(Registers reg) const;
		void writeReg16(Registers reg, uint16_t val);

		uint16_t m_regPC = 0;
		uint16_t m_regSP = 0;

		uint16_t m_regAF = 0;
		uint16_t m_regBC = 0;
		uint16_t m_regDE = 0;
		uint16_t m_regHL = 0;

		uint16_t m_cyclesToWait = 0;

		bool m_stop = false;

		static constexpr size_t RAM_BYTES = 8 * 1024;

		std::vector<uint8_t> m_ram = std::vector<uint8_t>(RAM_BYTES, 0u);
		std::vector<uint8_t> m_vram = std::vector<uint8_t>(RAM_BYTES, 0u);

		Cart& m_cart;

	};
}