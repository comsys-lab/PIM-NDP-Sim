#ifndef RAMULATOR_DRAM_LAMBDAS_ROWOPEN_H
#define RAMULATOR_DRAM_LAMBDAS_ROWOPEN_H

#include <spdlog/spdlog.h>

namespace Ramulator {
namespace Lambdas {
namespace RowOpen {
namespace Bank {
  template <class T>
  bool RDWR(typename T::Node* node, int cmd, int target_id, Clk_t clk) {
    switch (node->m_state)  {
      case T::m_states["Closed"]: return false;
      case T::m_states["Opened"]: return true;
      case T::m_states["Refreshing"]: return false;
      default: {
        spdlog::error("[RowHit::Bank] Invalid bank state for an RD/WR command!");
        std::exit(-1);      
      }
    }
  }
}       // namespace Bank

namespace BankGroup {
  template <class T>
  bool RD4(typename T::Node* node, int cmd, int target_id, Clk_t clk) {
    for (auto bank : node->m_child_nodes) {
      if (bank->m_state != T::m_states["Opened"]) { return false; }
    }
    return true;
  }
}       // namespace BankGroup

namespace Channel {
  template <class T>
  bool RDWR16(typename T::Node* node, int cmd, int target_id, Clk_t clk) {
    if constexpr (T::m_levels["bank"] - T::m_levels["channel"] == 2) {
      for (auto bg : node->m_child_nodes) {
        for (auto bank : bg->m_child_nodes) {
          if (bank->m_state != T::m_states["Opened"]) { return false; }
        }
      }
    } else if constexpr (T::m_levels["bank"] - T::m_levels["channel"] == 3) {
      for (auto pc : node->m_child_nodes) {
        for (auto bg : pc->m_child_nodes) {
          for (auto bank : bg->m_child_nodes) {
            if (bank->m_state != T::m_states["Opened"]) { return false; }
          }
        }
      }
    }
    return true;
  }
}       // namespace Channel
}       // namespace RowOpen
}       // namespace Lambdas
};      // namespace Ramulator

#endif  // RAMULATOR_DRAM_LAMBDAS_ROWOPEN_H