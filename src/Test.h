#pragma once
#include "Base.h"
#include "Emulator.h"

namespace ez {

class Tester {
  public:
    Tester();
    bool test_all();

  private:
    Emulator make_emulator();
    bool test_flags();
    bool test_regs();
    bool test_inc_dec();
    bool test_push_pop();
    bool test_io_reg();

    std::unique_ptr<Cart> m_cart;
};
} // namespace ez