#pragma once
#include "base.h"

namespace ez
{
	class Emulator
	{
	public:
		Emulator(const std::string& path);

	private:
		std::string m_path;

	};
}