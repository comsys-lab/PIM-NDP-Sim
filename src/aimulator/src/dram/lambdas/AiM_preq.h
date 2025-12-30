#ifndef RAMULATOR_DRAM_LAMBDAS_PREQ_H
#define RAMULATOR_DRAM_LAMBDAS_PREQ_H

#include <spdlog/spdlog.h>
// AiM
// #include <base/AiM_request.h>

namespace Ramulator {
namespace Lambdas {
namespace Preq {
namespace Bank {
  template <class T>
  int RequireRowOpen(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
    switch (node->m_state) {
      case T::m_states["Closed"]: return T::m_commands["ACT"];
      case T::m_states["Opened"]:
        if (node->m_row_state.find(addr_h[T::m_levels["row"]]) != node->m_row_state.end()) { return cmd; }
        else { return T::m_commands["PRE"]; }
      case T::m_states["Refreshing"]: return T::m_commands["ACT"];
      default: {
        spdlog::error("[Preq::Bank] Invalid bank state for an RD/WR command!");
        std::exit(-1);
      }
    }
  };

  template <class T>
  int RequireBankClosed(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
    switch (node->m_state) {
      case T::m_states["Closed"]: { return cmd; }
      case T::m_states["Opened"]: { return T::m_commands["PRE"]; }
      case T::m_states["Refreshing"]: { return cmd; }
      default: {
        spdlog::error("[Preq::Bank] Invalid bank state for an RD/WR command!");
        std::exit(-1);
      }
    }
  };
}       // namespace Bank

namespace BankGroup {
  template <class T>
  int RequireAllRowsOpen(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
    int num_open_banks = 0;
    const int num_banks = node->m_child_nodes.size();

    // Iterate over all banks in this bank group to assess their collective state
    for (auto bank : node->m_child_nodes) {
      switch (bank->m_state) {
        case T::m_states["Opened"]:
          // The bank is open. Check if it's open to the correct row.
          if (bank->m_row_state.find(addr_h[T::m_levels["row"]]) != bank->m_row_state.end()) { num_open_banks++; }
          // This bank is open to the WRONG row. A conflict exists.
          // The entire group must be precharged to resolve it.
          else { return T::m_commands["PRE4_BG"]; }
          break;
        // This bank is closed, which is a valid, non-conflicting state.
        // We just continue checking the other banks.
        case T::m_states["Closed"]: break;
        // A bank is refreshing - return the same command to indicate "not ready yet"
        // Timing constraints (REFab -> ACT4_BG with latency nRFCab) will enforce the wait
        // Once REFab_end executes and banks return to "Closed", this will resolve to ACT4_BG
        case T::m_states["Refreshing"]: return cmd;
        default:
          spdlog::error("[Preq::BankGroup] Invalid bank state for the 4-bank AiM commands applied to the bankgroup!");
          std::exit(-1);
      }
    }

    // After checking all banks, determine the correct group-level prerequisite
    // IDEAL CASE: All banks are already open to the correct row
    if (num_open_banks == num_banks) { return cmd; }
    // ALL CLOSED CASE: No banks were open to the correct row, and we already verified
    // that no banks are open to the wrong row. Thus, all banks are closed.
    else if (num_open_banks == 0) {
      return T::m_commands["ACT4_BG"];
    }
    // MIXED CASE: Some banks are correctly open, others are closed.
    // To perform a parallel operation, we need a uniform state.
    // The prerequisite is to precharge the whole group to get all banks to the "Closed" state.
    else { return T::m_commands["PRE4_BG"]; }
  };

  template <class T>
  int RequireAllBanksClosed(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
    if constexpr (T::m_levels["bank"] - T::m_levels["bankgroup"] == 1) {
      for (auto bank : node->m_child_nodes) {
        if (bank->m_state == T::m_states["Closed"]) { continue; }
        else if (bank->m_state == T::m_states["Refreshing"]) { return cmd; }
        else { return T::m_commands["PRE4_BG"]; }
      }
    } else {
      static_assert(
        false_v<T>,
        "[Preq::BankGroup] Unsupported organization. Please write your own RequireAllBanksClosed function."
      );
    }
    return cmd;
  };
}       // namespace BankGroup

namespace Rank {
  /*** ONLY For the LPDDR5 ***/
  template <class T>
  int RequireAllRowsOpen(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
    int num_banks = 0;
    int num_closed_banks = 0;
    int num_preopen_banks = 0;
    int num_open_banks = 0;

    // This inner lambda checks a single bank for conflicts.
    // It returns:
    //   -1: Bank is refreshing (special case - caller should return cmd to signal "not ready yet")
    //    0: Conflict detected (wrong row open - caller should return PREA)
    //    1: No conflict (bank is in acceptable state)
    auto is_target_row_and_not_ref = [&](typename T::Node* bank) -> int {
      num_banks++;
      switch (bank->m_state) {
        case T::m_states["Opened"]:
          if (bank->m_row_state.find(addr_h[T::m_levels["row"]]) != bank->m_row_state.end()) { num_open_banks++; }
          // CONFLICT: Wrong row is fully open.
          else { return 0; }
          break;
        case T::m_states["Pre-Opened"]:
          // Also check Pre-Opened for LPDDR5
          // Bank is open. Check if it's the correct row.
          if (bank->m_row_state.find(addr_h[T::m_levels["row"]]) != bank->m_row_state.end()) { num_preopen_banks++; }
          // CONFLICT: Wrong row is pre-opened.
          else { return 0; }
          break;
        case T::m_states["Closed"]:
          num_closed_banks++;
          break;
        // REFRESHING: Bank is refreshing - signal to caller to return cmd (not ready yet)
        case T::m_states["Refreshing"]:
          return -1;
        // Any other state is a conflict
        default: return 0;
      }
      return 1;
    };

    // Iterate through all banks within this rank.
    // The LPDDR5 hierarchy is Rank -> BankGroup -> Bank.
    // The compiler will use this path since `T::m_levels["bank"] - T::m_levels["rank"]` is 2.
    if constexpr (T::m_levels["bank"] - T::m_levels["rank"] == 2) {
      for (auto bg : node->m_child_nodes) {
        for (auto bank : bg->m_child_nodes) {
          int check_result = is_target_row_and_not_ref(bank);
          if (check_result == -1) { return cmd; }  // Bank is refreshing - not ready yet
          else if (check_result == 0) { return T::m_commands["PREA"]; }  // Conflict - precharge needed
        }
      }
    } else {
      // Fallback for flat rank->bank hierarchies
      for (auto bank : node->m_child_nodes) {
        int check_result = is_target_row_and_not_ref(bank);
        if (check_result == -1) { return cmd; }  // Bank is refreshing - not ready yet
        else if (check_result == 0) { return T::m_commands["PREA"]; }  // Conflict - precharge needed
      }
    }

    // After checking all banks without finding a hard conflict, decide the next step.
    // Edge case
    if (num_banks == 0) { return cmd; }

    // IDEAL CASE: All banks in the rank are already open to the correct row.
    if (num_open_banks == num_banks) {
      if (node->m_final_synced_cycle < clk) {
        if (cmd == T::m_commands["WRAFLUT"] || cmd == T::m_commands["WRBK"]) { return T::m_commands["CASWR"]; }
        else { return T::m_commands["CASRD"]; }
      } else { return cmd; }
    }
    // ALL CLOSED CASE: No banks were correctly open, and no conflicts were found.
    // The next step is to activate them all. We assume a command like 'ACTA' exists
    // that the controller would translate into a sequence of ACT commands.
    else if (num_preopen_banks == num_banks) { return T::m_commands["ACT16-2"]; }
    else if (num_closed_banks == num_banks) { return T::m_commands["ACT16-1"]; }
    // TODO: Handling ACTAF commands
    else { return T::m_commands["PREA"]; }
  };

  template <class T>
  int RequireAllBanksClosed(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
    if constexpr (T::m_levels["bank"] - T::m_levels["rank"] == 1) {
      for (auto bank: node->m_child_nodes) {
        if (bank->m_state == T::m_states["Closed"]) { continue; }
        else if (bank->m_state == T::m_states["Refreshing"]) { return cmd; }
        else { return T::m_commands["PREA"]; }
      }
    } else if constexpr (T::m_levels["bank"] - T::m_levels["rank"] == 2) {
      for (auto bg : node->m_child_nodes) {
        for (auto bank: bg->m_child_nodes) {
          if (bank->m_state == T::m_states["Closed"]) { continue; }
          else if(bank->m_state == T::m_states["Refreshing"]) { return cmd; }
          else { return T::m_commands["PREA"]; }
        }
      }
    }
    return cmd;
  };

  template <class T>
  int RequireSameBanksClosed(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
    bool all_banks_ready = true;
    for (auto bg : node->m_child_nodes) {
      for (auto bank : bg->m_child_nodes) {
        if (bank->m_node_id == addr_h[T::m_levels["bank"]]) {
          all_banks_ready &= (bank->m_state == T::m_states["Closed"]) || (bank->m_state == T::m_states["Refreshing"]);
        }
      }
    }
    if (all_banks_ready) { return cmd; }
    else { return T::m_commands["PREsb"]; }
  };
}       // namespace Rank

namespace Channel {
  // AiM
  template <class T>
  int RequireAllRowsOpen(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
    int num_banks = 0;
    int num_open_banks = 0;

    // This lambda contains the core logic for checking a single bank's state.
    // It returns:
    //   -1: Bank is refreshing (special case - caller should return cmd to signal "not ready yet")
    //    0: Conflict detected (wrong row open - caller should return PREA)
    //    1: No conflict (bank is in acceptable state)
    auto is_target_row_or_closed = [&](typename T::Node* bank) -> int {
      num_banks++;
      switch (bank->m_state) {
        case T::m_states["Opened"]:
          // Bank is open. Check if it's the correct row.
          if (bank->m_row_state.find(addr_h[T::m_levels["row"]]) != bank->m_row_state.end()) { num_open_banks++; }
          // Conflict: Bank is open to the WRONG row. Must precharge everything.
          else { return 0; }
          break;
        // This bank is closed. This is a valid, non-conflicting state.
        case T::m_states["Closed"]: break;
        // REFRESHING: Bank is refreshing - signal to caller to return cmd (not ready yet)
        case T::m_states["Refreshing"]: return -1;
        default:
          spdlog::error("[Preq::Channel] Invalid bank state encountered!");
          std::exit(-1);
      }
      return 1;
    };

    // Statically choose the correct traversal path based on the DRAM hierarchy.
    // The compiler will discard the unused branch.
    if constexpr (T::m_levels["bank"] - T::m_levels["channel"] == 2) {
      for (auto bg : node->m_child_nodes) {
        for (auto bank : bg->m_child_nodes) {
          int check_result = is_target_row_or_closed(bank);
          if (check_result == -1) { return cmd; }  // Bank is refreshing - not ready yet
          else if (check_result == 0) { return T::m_commands["PREA"]; }  // Conflict - precharge needed
        }
      }
    } else if constexpr (T::m_levels["bank"] - T::m_levels["channel"] == 3) {
      for (auto pc : node->m_child_nodes) {
        for (auto bg : pc->m_child_nodes) {
          for (auto bank : bg->m_child_nodes) {
            int check_result = is_target_row_or_closed(bank);
            if (check_result == -1) { return cmd; }  // Bank is refreshing - not ready yet
            else if (check_result == 0) { return T::m_commands["PREA"]; }  // Conflict - precharge needed
          }
        }
      }
    }

    // After checking all banks without finding a conflict, decide the final prerequisite.
    // Edge case for a channel with no banks.
    if (num_banks == 0) { return cmd; }

    // IDEAL CASE: All banks in the channel are already open to the correct row.
    if (num_open_banks == num_banks) { return cmd; }
    // ALL CLOSED CASE: No banks were correctly open, and no conflicts were found.
    // This means all banks are in the "Closed" state.
    else if (num_open_banks == 0) {
      return T::m_commands["ACT16"];
    }
    // MIXED CASE: Some banks are correctly open, and the rest are closed.
    // To get to a uniform state for a parallel command, we must precharge everything.
    else { return T::m_commands["PREA"]; }
  };

  template <class T>
  int RequireAllBanksClosed(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
    if constexpr (T::m_levels["bank"] - T::m_levels["channel"] == 2) {
      for (auto bg : node->m_child_nodes) {
        for (auto bank: bg->m_child_nodes) {
          if (bank->m_state == T::m_states["Closed"]) { continue; }
          else if (bank->m_state == T::m_states["Refreshing"]) { return cmd; }
          else { return T::m_commands["PREA"]; }
        }
      }
    } else if constexpr (T::m_levels["bank"] - T::m_levels["channel"] == 3) {
      for (auto pc : node->m_child_nodes) {
        for (auto bg : pc->m_child_nodes) {
          for (auto bank: bg->m_child_nodes) {
            if (bank->m_state == T::m_states["Closed"]) { continue; }
            else if (bank->m_state == T::m_states["Refreshing"]) { return cmd; }
            else { return T::m_commands["PREA"]; }
          }
        }
      }
    }
    return cmd;
  };
}       // namespace Channel
}       // namespace Preq
}       // namespace Lambdas
};      // namespace Ramulator

#endif  // RAMULATOR_DRAM_LAMBDAS_PREQ_H