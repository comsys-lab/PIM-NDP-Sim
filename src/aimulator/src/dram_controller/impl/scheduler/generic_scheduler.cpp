#include <vector>

#include "base/base.h"
#include "dram_controller/controller.h"
#include "dram_controller/scheduler.h"

namespace Ramulator {

class FRFCFS : public IScheduler, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IScheduler, FRFCFS, "FRFCFS", "FRFCFS DRAM Scheduler.")

  public:
    void init() override {};

    void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
      m_dram = cast_parent<IDRAMController>()->m_dram;
    };

    ReqBuffer::iterator compare(ReqBuffer::iterator req1, ReqBuffer::iterator req2) override {
      bool ready1 = m_dram->check_ready(req1->command, req1->addr_h);
      bool ready2 = m_dram->check_ready(req2->command, req2->addr_h);

      if (ready1 ^ ready2) {
        if (ready1) {
          return req1;
        } else {
          return req2;
        }
      }

      // Fallback to FCFS
      if (req1->arrive <= req2->arrive) {
        return req1;
      } else {
        return req2;
      }
    }

    ReqBuffer::iterator get_best_request(ReqBuffer& buffer) override {
      if (buffer.size() == 0) {
        return buffer.end();
      }

      for (auto& req : buffer) {
        req.command = m_dram->get_preq_command(req.final_command, req.addr_h);
      }

      auto candidate = buffer.begin();
      for (auto next = std::next(buffer.begin(), 1); next != buffer.end(); next++) {
        candidate = compare(candidate, next);
      }
      return candidate;
    }

    ReqBuffer::iterator get_best_aim_request(ReqBuffer& buffer) override {
      if (buffer.size() == 0) {
        return buffer.end();
      }

      // Prepare all requests: get their prerequisite commands
      for (auto& req : buffer) {
        req.command = m_dram->get_preq_command(req.final_command, req.addr_h);
      }

      // Get the scope of the first request (establishes the "scope group")
      auto first_it = buffer.begin();
      Level_t first_scope = m_dram->m_command_action_scope(first_it->command);

      // Channel scope (level 0) -> strict in-order
      if (first_scope == 0) {
        if (m_dram->check_ready(first_it->command, first_it->addr_h)) {
          return first_it;
        }
        return buffer.end();  // Blocked
      }

      // For other scopes, find best ready request with same scope
      ReqBuffer::iterator best = buffer.end();
      for (auto it = buffer.begin(); it != buffer.end(); it++) {
        Level_t it_scope = m_dram->m_command_action_scope(it->command);

        // Block if different scope (must maintain order between scope groups)
        if (it_scope != first_scope) {
          break;
        }

        // Check if this request is ready
        if (m_dram->check_ready(it->command, it->addr_h)) {
          if (best == buffer.end()) {
            best = it;
          } else {
            // FCFS tiebreaker: pick earlier arrival
            if (it->arrive < best->arrive) {
              best = it;
            }
          }
        }
      }
      return best;
    }

  private:
    IDRAM* m_dram;

};

}       // namespace Ramulator
