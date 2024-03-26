#pragma once
#include "Base.h"
#include "Emulator.h"

namespace ez {

class Tester {
  public:
    Tester() = default;
    bool test_all();

  private:
    void reset_emulator();
    bool test_flags();
    bool test_regs();
    bool test_inc_dec();

    std::unique_ptr<Cart> m_cart;
    std::unique_ptr<Emulator> m_emu;
};
} // namespace ez