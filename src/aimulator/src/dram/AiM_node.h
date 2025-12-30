#ifndef RAMULATOR_DRAM_NODE_H
#define RAMULATOR_DRAM_NODE_H

#include <vector>
#include <map>
#include <deque>
#include <functional>
#include <concepts>

#include "base/type.h"
#include "dram/spec.h"

namespace Ramulator {

class IDRAM;

template<typename T>
concept IsDRAMSpec = requires(T t) { 
  typename T::Node;
};

// CRTP class defnition is not complete, so we cannot have something nice like:
// template<typename T>
// concept IsDRAMSpec = std::is_base_of_v<IDRAM, T> && requires(T t) { 
//   typename T::Node; 
// };


/**
 * @brief     CRTP-ish (?) base class of a DRAM Device Node
 * 
 */
template<IsDRAMSpec T>
struct DRAMNodeBase {
  using NodeType = typename T::Node;
  NodeType* m_parent_node = nullptr;
  std::vector<NodeType*> m_child_nodes;

  T* m_spec = nullptr;

  // The level of this node in the organization hierarchy
  int m_level = -1;
  // The id of this node at this level
  int m_node_id = -1;
  // The size of the node (e.g., how many rows in a bank)
  int m_size = -1;
  // The state of the node
  int m_state = -1;
  // The next cycle that each command can be issued again at this level
  std::vector<Clk_t> m_cmd_ready_clk;
  // Issue-history of each command at this level
  std::vector<std::deque<Clk_t>> m_cmd_history;

  using RowId_t = int;
  using RowState_t = int;
  // The state of the rows, if I am a bank-ish node
  std::map<RowId_t, RowState_t> m_row_state;

  DRAMNodeBase(T* spec, NodeType* parent, int level, int id):
  m_spec(spec), m_parent_node(parent), m_level(level), m_node_id(id) {
    int num_cmds = T::m_commands.size();
    m_cmd_ready_clk.resize(num_cmds, -1);
    m_cmd_history.resize(num_cmds);
    for (int cmd = 0; cmd < num_cmds; cmd++) {
      int window = 0;
      for (const auto& t : spec->m_timing_cons[level][cmd]) {
        window = std::max(window, t.window);
      }
      if (window != 0) { m_cmd_history[cmd].resize(window, -1); }
      else { m_cmd_history[cmd].clear(); }
    }

    m_state = spec->m_init_states[m_level];

    // Recursively construct next levels
    int next_level = level + 1;
    int last_level = T::m_levels["row"];
    if (next_level == last_level) { return; }
    else {
      int next_level_size = m_spec->m_organization.count[next_level];
      if (next_level_size == 0) { return; }
      else {
        for (int i = 0; i < next_level_size; i++) {
          NodeType* child = new NodeType(spec, static_cast<NodeType*>(this), next_level, i);
          static_cast<NodeType*>(this)->m_child_nodes.push_back(child);
        }
      }
    }
  };

  void update_states(int command, const AddrHierarchy_t& addr_h, Clk_t clk) {
    // 1. Execute the action lambda if one is defined for the current node level.
    if (m_spec->m_actions[m_level][command]) {
      // update the state machine at this level
      m_spec->m_actions[m_level][command](static_cast<NodeType*>(this), command, addr_h, clk); 
    }

    // 2. Determine if we should STOP the recursion.
    const int action_scope = m_spec->m_command_action_scope[command];

    // The recursion should stop if:
    // a) A multi-bank action was just performed. The action lambda handles all children states,
    //    so no further recursion is needed.
    // b) We have reached the deepest level required by the command's addressing.
    // c) We are at a leaf node in the hierarchy.
    if ((action_scope != -1 && m_level == action_scope) ||
        (m_level == m_spec->m_command_addressing_level[command]) ||
        !m_child_nodes.size()) {
      return;
    }

    // 3. RECURSE to the next level on the single path.
    //    This logic is only reached for commands that need to propagate state changes
    //    down a single path (like a standard ACT needing to update a row's state),
    //    and only when we are above the command's final addressing level.
    int child_id = addr_h[m_level + 1];
    if (child_id < 0) { return; }

    m_child_nodes[child_id]->update_states(command, addr_h, clk);
  };

  void update_powers(int command, const AddrHierarchy_t& addr_h, Clk_t clk) {
    if (!m_spec->m_drampower_enable)
      return;

    int child_id = addr_h[m_level+1];
    if (m_spec->m_powers[m_level][command]) {
      // update the power model at this level
      m_spec->m_powers[m_level][command](static_cast<NodeType*>(this), command, addr_h, clk);
    }
    if (m_level == m_spec->m_command_addressing_level[command] || !m_child_nodes.size()) {
      // stop recursion: updated all levels
      return; 
    }
    // recursively update child nodes
    if (child_id == -1){
      for (auto child : m_child_nodes) {
        child->update_powers(command, addr_h, clk);
      }
    } else {
      m_child_nodes[child_id]->update_powers(command, addr_h, clk);
    }
  };

  void update_timing(int command, const AddrHierarchy_t& addr_h, Clk_t clk) {
    /************************************************
     *         Update Sibling Node Timing
     ***********************************************/
    if (m_node_id != addr_h[m_level] && addr_h[m_level] != -1) {
      for (const auto& t : m_spec->m_timing_cons[m_level][command]) {
        if (!t.sibling) {
          // not sibling timing parameter
          continue;
        }

        // update earliest schedulable time of every command
        Clk_t future = clk + t.val;
        m_cmd_ready_clk[t.cmd] = std::max(m_cmd_ready_clk[t.cmd], future);
      }
      // stop recursion
      return;
    }

    /************************************************
     *          Update Target Node Timing
     ***********************************************/
    // Update history
    if (m_cmd_history[command].size()) {
      m_cmd_history[command].pop_back();
      m_cmd_history[command].push_front(clk); 
    }

    for (const auto& t : m_spec->m_timing_cons[m_level][command]) {
      if (t.sibling) {
        continue;
      }

      // Get the oldest history
      Clk_t past = m_cmd_history[command][t.window-1];
      if (past < 0) {
        // not enough history
        continue; 
      }

      // update earliest schedulable time of every command
      Clk_t future = past + t.val;
      m_cmd_ready_clk[t.cmd] = std::max(m_cmd_ready_clk[t.cmd], future);
    }

    /************************************************
     *           Conditional Recursion
     ************************************************/

    if (!m_child_nodes.size()) {
      // stop recursion: updated all levels
      return; 
    }

    const int action_scope = m_spec->m_command_action_scope[command];

    if (action_scope != -1 && m_level >= action_scope) {
      // FAN-OUT RECURSION:
      // This is a multi-bank command and we are at or below its action scope.
      // e.g., We are a BankGroup processing ACT4_BG, so we must update all our Banks.
      // Or we are a Bank processing ACT4_BG (called from our parent BG), so we must update our Rows.
      // Therefore, broadcast the update to all children.
      for (auto child : m_child_nodes) {
        child->update_timing(command, addr_h, clk);
      }
    } else {
      // SINGLE-PATH RECURSION:
      // This is a standard single-address command (like RD/WR), or we are
      // at a level above a multi-bank command's action scope.
      // In either case, we follow the single path specified by the address.
      int child_id = addr_h[m_level + 1];
      if (child_id < 0 || child_id >= m_child_nodes.size()) {
        spdlog::error("[update_timing] Invalid child_id {} at level {}. child_nodes size: {}", 
                      child_id, m_level, m_child_nodes.size());
        return;
      }
      m_child_nodes[child_id]->update_timing(command, addr_h, clk);
    }
  };

  int get_preq_command(int command, const AddrHierarchy_t& addr_h, Clk_t m_clk) {
    // 1. Check if a prerequisite commands is defined for the current level.
    if (m_spec->m_preqs[m_level][command]) {
      int preq_cmd = m_spec->m_preqs[m_level][command](static_cast<NodeType*>(this), command, addr_h, m_clk);
      // If the lambda returns a valid command (even the original one), it has made a decision.
      if (preq_cmd != -1) {
        // stop recursion: there is a prerequisite at this level
        return preq_cmd;
      }
    }

    // 2. Determine if we should stop the recursion.
    const int action_scope = m_spec->m_command_action_scope[command];
    // We stop if
    // - We are at the level where a multi-bank command's action is defined.
    // - We are at a leaf node with no more children to check.
    if ((action_scope != -1 && m_level == action_scope) || !m_child_nodes.size()) {
      // If we reached here, it means no prerequisite was found at any level on the path.
      // The original command is ready to go.
      return command;
    }

    // 3. Single path recursion to the next level.
    // - if we are above a command's action scope, OR
    // - if it's a standard single-address command.
    int child_id = addr_h[m_level + 1];
    if (child_id < 0 || child_id >= m_child_nodes.size()) {
      spdlog::error("[get_preq_command] Invalid child_id {} at level {}. child_nodes size: {}", 
                    child_id, m_level, m_child_nodes.size());
      return -1;
    }
    return m_child_nodes[child_id]->get_preq_command(command, addr_h, m_clk);
  };

  bool check_ready(int command, const AddrHierarchy_t& addr_h, Clk_t clk) {
    // 1. Check local timing constraints at the current level.
    if (m_cmd_ready_clk[command] != -1 && clk < m_cmd_ready_clk[command]) {
      // A timing constraint at this level prevents the command.
      // stop recursion: the check failed at this level
      return false;
    }

    // 2. Stop condition: Check passed at all levels.
    if (m_level == m_spec->m_command_addressing_level[command] || !m_child_nodes.size()) {
      return true;
    }

    // 3. Determine the recursion policy (Single-Path vs. Fan-Out).
    // Check if the current command has a defined multi-node action scope.
    // The LUT will return a special value (e.g., -1) if the command is not a group command.
    const int action_scope = m_spec->m_command_action_scope[command];

    if (m_level == action_scope) {
      // FAN-OUT RECURSION:
      // We are at the level where this command becomes a group action.
      // e.g., We are a 'BankGroup' node and the command is 'ACT4_BG'.
      // The command is ready only if it is ready for ALL children.
      bool all_children_ready = true;
      for (auto child : m_child_nodes) {
        if (!child->check_ready(command, addr_h, clk)) {
          all_children_ready = false;
          break; // Optimization: one failure is enough to make the whole group not ready.
        }
      }
      return all_children_ready;
    } else {
      // SINGLE-PATH RECURSION:
      // This is either a standard command (like RD/WR/ACT) or we are at a level
      // above the command's action scope (e.g., at the Channel for an ACT4_BG).
      // In either case, we follow the single path specified by the address.
      int child_id = addr_h[m_level + 1];
      if (child_id < 0 || child_id >= m_child_nodes.size()) {
        spdlog::error("[check_ready] Invalid child_id {} at level {}. child_nodes size: {}", 
                      child_id, m_level, m_child_nodes.size());
        return false;
      }
      return m_child_nodes[child_id]->check_ready(command, addr_h, clk);
    }
  };

  bool check_rowbuffer_hit(int command, const AddrHierarchy_t& addr_h, Clk_t m_clk) {
    // TODO: Optimize this by just checking the bank-levels? Have a dedicated bank structure?
    int child_id = addr_h[m_level+1];
    if (m_spec->m_rowhits[m_level][command]) {
      // stop recursion: there is a row hit at this level
      return m_spec->m_rowhits[m_level][command](static_cast<NodeType*>(this), command, child_id, m_clk);  
    }

    if (!m_child_nodes.size()) {
      // stop recursion: there were no row hits at any level
      return false; 
    }

    // recursively check for row hits at my child
    return m_child_nodes[child_id]->check_rowbuffer_hit(command, addr_h, m_clk);
  };
  
  bool check_node_open(int command, const AddrHierarchy_t& addr_h, Clk_t m_clk) {
    int child_id = addr_h[m_level+1];
    if (m_spec->m_rowopens[m_level][command])
      // stop recursion: there is a row open at this level
      return m_spec->m_rowopens[m_level][command](static_cast<NodeType*>(this), command, child_id, m_clk);  

    if (!m_child_nodes.size())
      // stop recursion: there were no row hits at any level
      return false;

    // recursively check for row hits at my child
    return m_child_nodes[child_id]->check_node_open(command, addr_h, m_clk);
  }
};

template<class T>
using ActionFunc_t = std::function<void(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk)>;
template<class T>
using PreqFunc_t   = std::function<int (typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk)>;
template<class T>
using RowhitFunc_t = std::function<bool(typename T::Node* node, int cmd, int target_id, Clk_t clk)>;
template<class T>
using RowopenFunc_t = std::function<bool(typename T::Node* node, int cmd, int target_id, Clk_t clk)>;
template<class T>
using PowerFunc_t = std::function<void(typename T::Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk)>;

template<typename T>
using FuncMatrix = std::vector<std::vector<T>>;

// TODO: Enable easy syntax for FuncMatrix lookup
// template<typename T, int N, int M>
// class FuncMatrix : public std::array<std::array<T, M>, N> {

// };

}        // namespace Ramulator

#endif   // RAMULATOR_DRAM_NODE_H
