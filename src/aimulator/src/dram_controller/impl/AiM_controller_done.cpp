#include "dram_controller/controller.h"
#include "memory_system/memory_system.h"
// AiM
#include "base/AiM_request.h"
#include <cstdio>
#include <string>

#include <iomanip>

namespace Ramulator {

class AiMController final : public IDRAMController, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IDRAMController, AiMController, "AiMController", "AiM controller.");

  public:
    void init() override {
      m_wr_low_watermark =  param<float>("wr_low_watermark").desc("Threshold for switching back to read mode.").default_val(0.2f);
      m_wr_high_watermark = param<float>("wr_high_watermark").desc("Threshold for switching to write mode.").default_val(0.8f);
      // m_clock_ratio = param<uint>("clock_ratio").required();

      m_scheduler = create_child_ifce<IScheduler>();
      // m_refresh = create_child_ifce<IRefreshManager>();
      // m_rowpolicy = create_child_ifce<IRowPolicy>();

      if (m_config["plugins"]) {
        YAML::Node plugin_configs = m_config["plugins"];
        for (YAML::iterator it = plugin_configs.begin(); it != plugin_configs.end(); ++it) {
          m_plugins.push_back(create_child_ifce<IControllerPlugin>(*it));
        }
      }
    };

    void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
      m_dram = memory_system->get_ifce<IDRAM>();
      m_bank_addr_idx = m_dram->m_levels("bank");
      m_priority_buffer.max_size = 512 * 3 + 32;
      auto existing_logger = spdlog::get("AiMController[" + std::to_string(m_channel_id) + "]");
      // if (existing_logger) {
      //   m_logger = existing_logger;
      // } else {
      //   m_logger = Logging::create_logger("AiMController[" + std::to_string(m_channel_id) + "]");
      // }

      // m_num_cores = frontend->get_num_cores();
      // s_read_row_hits_per_core.resize(m_num_cores, 0);
      // s_read_row_misses_per_core.resize(m_num_cores, 0);
      // s_read_row_conflicts_per_core.resize(m_num_cores, 0);

      // for (const auto type : {Request::Type::Read, Request::Type::Write}) {
      //   s_num_RW_cycles[type] = 0;
      //   register_stat(s_num_RW_cycles[type])
      //     .name(fmt::format("CH{}_{}_cycles", m_channel_id, str_type_name(type)));
      // }
      for (const auto type : {Request::Type::WR_GB, Request::Type::WR_MAC, Request::Type::WR_BIAS,
                              Request::Type::RD_MAC, Request::Type::RD_AF}) {
        s_num_AiM_no_bank_cycles[type] = 0;
        // register_stat(s_num_AiM_no_bank_cycles[type])
        //   .name(fmt::format("CH{}_AiM_{}_cycles", m_channel_id, str_type_name(type)));
      }
      for (const auto type : {Request::Type::MAC_SBK, Request::Type::AF_SBK, Request::Type::COPY_BKGB, Request::Type::COPY_GBBK,
                              Request::Type::MAC_4BK_INTRA_BG, Request::Type::AF_4BK_INTRA_BG, Request::Type::EWMUL, Request::Type::EWADD,
                              Request::Type::MAC_ABK, Request::Type::AF_ABK, Request::Type::WR_AFLUT, Request::Type::WR_BK}) {
        // Request::Type::MAC_4BK_INTER_BG, Request::Type::AF_4BK_INTER_BG,
        s_num_AiM_bank_cycles[type] = 0;
        // register_stat(s_num_AiM_bank_cycles[type])
        //   .name(fmt::format("CH{}_AiM_{}_cycles", m_channel_id, str_type_name(type)));
      }

      // for (int type = Request::Type::MAC_SBK; type < Request::Type::WR_GB; type++) {
      //   s_num_AiM_bank_cycles[type] = 0;
      //   register_stat(s_num_AiM_bank_cycles).name(fmt::format("CH{}_AiM_{}_cycles", m_channel_id, str_type_name(type))).desc(fmt::format("total number of AiM {} cycles", str_type_name(type)));
      // }

      // for (int type = Request::Type::WR_GB; type < Request::Type::UNKNOWN; type++) {
      //   s_num_AiM_no_bank_cycles[type] = 0;
      //   register_stat(s_num_AiM_no_bank_cycles).name(fmt::format("CH{}_AiM_{}_cycles", m_channel_id, str_type_name(type))).desc(fmt::format("total number of AiM {} cycles", str_type_name(type)));
      // }

      for (int cmd = 0; cmd < m_dram->m_commands.size(); cmd++) {
        s_num_commands[cmd] = 0;
        register_stat(s_num_commands[cmd])
          .name(fmt::format("CH{}_num_{}_commands", m_channel_id, std::string(m_dram->m_commands(cmd))));
      }

      register_stat(s_num_idle_cycles)
        .name(fmt::format("CH{}_idle_cycles", m_channel_id));
      register_stat(s_num_active_cycles)
        .name(fmt::format("CH{}_active_cycles", m_channel_id));
      register_stat(s_num_precharged_cycles)
        .name(fmt::format("CH{}_precharged_cycles", m_channel_id));

      // register_stat(s_row_hits).name("row_hits_{}", m_channel_id);
      // register_stat(s_row_misses).name("row_misses_{}", m_channel_id);
      // register_stat(s_row_conflicts).name("row_conflicts_{}", m_channel_id);
      // register_stat(s_read_row_hits).name("read_row_hits_{}", m_channel_id);
      // register_stat(s_read_row_misses).name("read_row_misses_{}", m_channel_id);
      // register_stat(s_read_row_conflicts).name("read_row_conflicts_{}", m_channel_id);
      // register_stat(s_write_row_hits).name("write_row_hits_{}", m_channel_id);
      // register_stat(s_write_row_misses).name("write_row_misses_{}", m_channel_id);
      // register_stat(s_write_row_conflicts).name("write_row_conflicts_{}", m_channel_id);
      
      // for (size_t core_id = 0; core_id < m_num_cores; core_id++) {
      //   register_stat(s_read_row_hits_per_core[core_id]).name("read_row_hits_core_{}", core_id);
      //   register_stat(s_read_row_misses_per_core[core_id]).name("read_row_misses_core_{}", core_id);
      //   register_stat(s_read_row_conflicts_per_core[core_id]).name("read_row_conflicts_core_{}", core_id);
      // }

      // register_stat(s_num_read_reqs).name("num_read_reqs_{}", m_channel_id);
      // register_stat(s_num_write_reqs).name("num_write_reqs_{}", m_channel_id);
      // register_stat(s_num_other_reqs).name("num_other_reqs_{}", m_channel_id);
      // register_stat(s_queue_len).name("queue_len_{}", m_channel_id);
      // register_stat(s_read_queue_len).name("read_queue_len_{}", m_channel_id);
      // register_stat(s_write_queue_len).name("write_queue_len_{}", m_channel_id);
      // register_stat(s_priority_queue_len).name("priority_queue_len_{}", m_channel_id);
      // register_stat(s_queue_len_avg).name("queue_len_avg_{}", m_channel_id);
      // register_stat(s_read_queue_len_avg).name("read_queue_len_avg_{}", m_channel_id);
      // register_stat(s_write_queue_len_avg).name("write_queue_len_avg_{}", m_channel_id);
      // register_stat(s_priority_queue_len_avg).name("priority_queue_len_avg_{}", m_channel_id);

      // register_stat(s_read_latency).name("read_latency_{}", m_channel_id);
      // register_stat(s_avg_read_latency).name("avg_read_latency_{}", m_channel_id);
    };

    bool send(Request& req) override {
      if (req.is_aim_req) {
        std::cerr << "[AiM Controller CH" << m_channel_id << " send()] type: " << req.type_id << " addr: 0x" << std::hex << req.addr
                  << " is_aim: " << req.is_aim_req << " aim_num_banks: " << std::dec << req.aim_num_banks << std::endl;
      }
      if (!req.is_aim_req) {
        if (m_aim_bank_buffer.size() != 0 || m_aim_no_bank_buffer.size() != 0) {
          std::cerr << "[AiM Controller CH" << m_channel_id << " send()] REJECTED: AiM buffer not empty" << std::endl;
          return false;
        }
        req.final_command = m_dram->m_request_translations(req.type_id);
      } else {
        if (m_write_buffer.size() != 0 || m_read_buffer.size() != 0) {
          std::cerr << "[AiM Controller CH" << m_channel_id << " send()] REJECTED: RW buffer not empty" << std::endl;
          return false;
        }
        // RD/WR excluded.
        req.final_command = m_dram->m_aim_req_translation(req.type_id - 2);
      }
      // std::cout << "[Ramulator (AiM Controller)] final_cmd: " << req.final_command << std::endl;

      // Forward existing write requests to incoming read requests
      if (req.type_id == Request::Type::Read) {
        auto compare_addr = [req](const Request& wreq) {
          return wreq.addr == req.addr;
        };
        if (std::find_if(m_write_buffer.begin(), m_write_buffer.end(), compare_addr) != m_write_buffer.end()) {
          // The request will depart at the next cycle
          req.depart = m_clk + 1;
          pending_reads.push(req);
          return true;
        }
      }

      // Else, enqueue them to corresponding buffer based on request type id
      bool is_success = false;
      req.arrive = m_clk;
      if (req.type_id == Request::Type::Read) {
        if (!m_read_buffer.enqueue(req)) {
          req.arrive = -1;
          throw std::runtime_error("Buffer (Read) is full!");
        }
      } else if (req.type_id == Request::Type::Write) {
        if (!m_write_buffer.enqueue(req)) {
          req.arrive = -1;
          throw std::runtime_error("Buffer (Write) is full!");
        }
      } else if (req.is_aim_req && req.aim_num_banks != 0) {
        if (!m_aim_bank_buffer.enqueue(req)) {
          req.arrive = -1;
          throw std::runtime_error("Buffer (AiM Bank Ops) is full!");
        }
        std::cerr << "[AiM Controller CH" << m_channel_id << " send()] Enqueued to m_aim_bank_buffer, size=" << m_aim_bank_buffer.size() << std::endl;
      } else if (req.is_aim_req && req.aim_num_banks == 0) {
        if (!m_aim_no_bank_buffer.enqueue(req)) {
          req.arrive = -1;
          throw std::runtime_error("Buffer (AiM No Bank Ops) is full!");
        }
        std::cerr << "[AiM Controller CH" << m_channel_id << " send()] Enqueued to m_aim_no_bank_buffer, size=" << m_aim_no_bank_buffer.size() << std::endl;
      } else {
        throw std::runtime_error("Invalid request type!");
      }

      return true;
    };

    bool priority_send(Request& req) override {
      if (!req.is_aim_req) {
        req.final_command = m_dram->m_request_translations(req.type_id);
      } else {
        req.final_command = m_dram->m_aim_req_translation(req.type_id);
      }

      bool is_success = false;
      is_success = m_priority_buffer.enqueue(req);
      return is_success;
    }

    void tick() override {
      // bool does_exist_any_req = m_read_buffer.size() + m_write_buffer.size() + m_aim_bank_buffer.size() + m_aim_no_bank_buffer.size();
      // if (!does_exist_any_req) {
      //   std::cout << "[AiMulator (Controller)] No memory request sent!" << std::endl;
      //   return;
      // }

      m_clk++;

      if (m_aim_bank_buffer.size() > 0 || m_aim_no_bank_buffer.size() > 0 ||
          pending_aim_bank.size() > 0 || pending_aim_no_bank.size() > 0) {
        std::cerr << "[AiM Controller CH" << m_channel_id << " tick()] clk=" << m_clk
                  << " aim_bank_buf=" << m_aim_bank_buffer.size()
                  << " aim_no_bank_buf=" << m_aim_no_bank_buffer.size()
                  << " pending_aim_bank=" << pending_aim_bank.size()
                  << " pending_aim_no_bank=" << pending_aim_no_bank.size() << std::endl;
      }

      // Update statistics
      // s_queue_len += m_read_buffer.size() + m_write_buffer.size() + m_priority_buffer.size() + pending_reads.size();
      // s_read_queue_len += m_read_buffer.size() + pending_reads.size();
      // s_write_queue_len += m_write_buffer.size();
      // s_priority_queue_len += m_priority_buffer.size();

      // 1. Serve completed reads
      serve_completed_reqs();

      // m_refresh->tick();

      // 2. Try to find a request to serve.
      ReqBuffer::iterator req_it;
      ReqBuffer* buffer = nullptr;
      bool request_found = schedule_request(req_it, buffer);
      if (m_aim_bank_buffer.size() > 0 || m_aim_no_bank_buffer.size() > 0 ||
          pending_aim_bank.size() > 0 || pending_aim_no_bank.size() > 0) {
        std::cerr << "[AiM Controller CH" << m_channel_id << " tick()] request_found=" << request_found << std::endl;
      }

      // 2.1 Take row policy action
      // m_rowpolicy->update(request_found, req_it);

      // 3. Update all plugins
      // for (auto plugin : m_plugins) {
      //   plugin->update(request_found, req_it);
      // }

      // 4. Finally, issue the commands to serve the request
      if (request_found) {
        // If we find a real request to serve
        // if (req_it->is_stat_updated == false) {
        //   update_request_stats(req_it);
        // }
        if (req_it->issue == -1) {
          req_it->issue = m_clk - 1;
        }
        m_dram->issue_command(req_it->command, req_it->addr_h);
        s_num_commands[req_it->command] += 1;

        // If we are issuing the last command, set depart clock cycle and move the request to the pending queue
        if (req_it->command == req_it->final_command) {
          int final_cmd_exec_time = m_dram->m_command_latencies(req_it->command);
          req_it->depart = m_clk + final_cmd_exec_time;
          std::cerr << "[AiM Controller CH" << m_channel_id << " tick()] Final command issued, type=" << req_it->type_id
                    << " addr=0x" << std::hex << req_it->addr << std::dec
                    << " depart=" << req_it->depart << std::endl;
          // Route completed requests to appropriate pending queues
          // IMPORTANT: Check is_aim_req to avoid routing refresh/system requests
          // (which may have type_id values that collide with AiM types) to AiM pending queues
          if (req_it->is_aim_req) {
            // AiM requests go to AiM pending queues
            switch (req_it->type_id) {
              case Request::Type::MAC_SBK:
              case Request::Type::AF_SBK:
              case Request::Type::COPY_BKGB:
              case Request::Type::COPY_GBBK:
              case Request::Type::MAC_4BK_INTRA_BG:
              case Request::Type::AF_4BK_INTRA_BG:
              case Request::Type::EWMUL:
              case Request::Type::EWADD:
              case Request::Type::MAC_ABK:
              case Request::Type::AF_ABK:
              case Request::Type::WR_AFLUT:
              case Request::Type::WR_BK:
                pending_aim_bank.push(*req_it);
                s_num_AiM_bank_cycles[req_it->type_id] += (m_clk - req_it->issue);
                break;
              case Request::Type::WR_GB:
              case Request::Type::WR_MAC:
              case Request::Type::WR_BIAS:
              case Request::Type::RD_MAC:
              case Request::Type::RD_AF:
                pending_aim_no_bank.push(*req_it);
                s_num_AiM_no_bank_cycles[req_it->type_id] += (m_clk - req_it->issue);
                break;
              // case Request::Type::MAC_4BK_INTER_BG:
              // case Request::Type::AF_4BK_INTER_BG:
              default:
                // Unknown AiM request type - should not happen
                break;
            }
          } else {
            // Non-AiM requests (Read, Write, Refresh, etc.)
            switch (req_it->type_id) {
              case Request::Type::Read:
                pending_reads.push(*req_it);
                s_num_RW_cycles[req_it->type_id] += (m_clk - req_it->issue);
                break;
              case Request::Type::Write:
                pending_writes.push_back(*req_it);
                s_num_RW_cycles[req_it->type_id] += (m_clk - req_it->issue);
                break;
              default:
                // Refresh and other system commands - no pending queue needed,
                // they complete immediately after final command is issued
                break;
            }
          }
          buffer->remove(req_it);
        } else {
          if (m_dram->m_command_meta(req_it->command).is_opening) {
            if (m_active_buffer.enqueue(*req_it)) {
              buffer->remove(req_it);
            }
          }
        }
      } else if (m_read_buffer.size() == 0 && m_write_buffer.size() == 0
                && m_aim_bank_buffer.size() == 0 && m_aim_no_bank_buffer.size() == 0) {
        s_num_idle_cycles++;
      }

      if (m_dram->m_open_rows[m_channel_id] == 0) {
        s_num_precharged_cycles++;
      } else {
        s_num_active_cycles++;
      }
    };

  private:
    // A queue for RD/WR requests that are about to finish (callback after RL)
    std::queue<Request> pending_reads;
    std::vector<Request> pending_writes;
    // std::vector<std::queue<Request>> pending(Request::Type::UNKNOWN+1);

    // Buffer for requests being served. This has the highest priority 
    ReqBuffer m_active_buffer;
    // Buffer for high-priority requests (e.g., maintenance like refresh).
    ReqBuffer m_priority_buffer;
    // Read request buffer
    ReqBuffer m_read_buffer;
    // Write request buffer
    ReqBuffer m_write_buffer;

    int m_bank_addr_idx = -1;

    float m_wr_low_watermark;
    float m_wr_high_watermark;
    bool  m_is_write_mode = false;

    // size_t s_row_hits = 0;
    // size_t s_row_misses = 0;
    // size_t s_row_conflicts = 0;
    // size_t s_read_row_hits = 0;
    // size_t s_read_row_misses = 0;
    // size_t s_read_row_conflicts = 0;
    // size_t s_write_row_hits = 0;
    // size_t s_write_row_misses = 0;
    // size_t s_write_row_conflicts = 0;

    // size_t m_num_cores = 0;
    // std::vector<size_t> s_read_row_hits_per_core;
    // std::vector<size_t> s_read_row_misses_per_core;
    // std::vector<size_t> s_read_row_conflicts_per_core;

    // size_t s_num_read_reqs = 0;
    // size_t s_num_write_reqs = 0;
    // size_t s_num_other_reqs = 0;
    // size_t s_queue_len = 0;
    // size_t s_read_queue_len = 0;
    // size_t s_write_queue_len = 0;
    // size_t s_priority_queue_len = 0;
    // float s_queue_len_avg = 0;
    // float s_read_queue_len_avg = 0;
    // float s_write_queue_len_avg = 0;
    // float s_priority_queue_len_avg = 0;

    // size_t s_read_latency = 0;
    // float s_avg_read_latency = 0;
    
    // Missing parts in Ramulator; Cycles
    std::map<int, int> s_num_commands;
    std::map<int, int> s_num_RW_cycles;
    int s_num_idle_cycles = 0;
    int s_num_active_cycles = 0;
    int s_num_precharged_cycles = 0;

    // AiM
    ReqBuffer m_aim_bank_buffer;
    ReqBuffer m_aim_no_bank_buffer;
    // in-order execution
    std::queue<Request> pending_aim_bank;
    std::queue<Request> pending_aim_no_bank;
    std::map<int, int> s_num_AiM_bank_cycles;
    std::map<int, int> s_num_AiM_no_bank_cycles;
    bool is_reg_RW_mode = false;

  private:
    /**
     * @brief    Helper function to check if a request is hitting an open row
     * @details
     * 
     */
    bool is_row_hit(ReqBuffer::iterator& req) {
      return m_dram->check_rowbuffer_hit(req->final_command, req->addr_h);
    }
    /**
     * @brief    Helper function to check if a request is opening a row
     * @details
     * 
    */
    bool is_row_open(ReqBuffer::iterator& req) {
      return m_dram->check_node_open(req->final_command, req->addr_h);
    }

    /**
     * @brief    
     * @details
     * 
     */
    void update_request_stats(ReqBuffer::iterator& req) {
      // req->is_stat_updated = true;

      // if (req->type_id == Request::Type::Read) {
      //   if (is_row_hit(req)) {
      //     s_read_row_hits++;
      //     s_row_hits++;
      //     if (req->source_id != -1) {
      //       s_read_row_hits_per_core[req->source_id]++;
      //     }
      //   } else if (is_row_open(req)) {
      //     s_read_row_conflicts++;
      //     s_row_conflicts++;
      //     if (req->source_id != -1) {
      //       s_read_row_conflicts_per_core[req->source_id]++;
      //     }
      //   } else {
      //     s_read_row_misses++;
      //     s_row_misses++;
      //     if (req->source_id != -1) {
      //       s_read_row_misses_per_core[req->source_id]++;
      //     }
      //   } 
      // } else if (req->type_id == Request::Type::Write) {
      //   if (is_row_hit(req)) {
      //     s_write_row_hits++;
      //     s_row_hits++;
      //   } else if (is_row_open(req)) {
      //     s_write_row_conflicts++;
      //     s_row_conflicts++;
      //   } else {
      //     s_write_row_misses++;
      //     s_row_misses++;
      //   }
      // }
      // TODO: Collect AiMISR stats
      // else if ((*req)->type_id == Request::Type::AiM_No_Bank) {
      // }
      // else {
      // }
    }

    /**
     * @brief    Helper function to serve the completed read requests
     * @details
     * This function is called at the beginning of the tick() function.
     * It checks the pending queue to see if the top request has received data from DRAM.
     * If so, it finishes this request by calling its callback and poping it from the pending queue.
     */
    void serve_completed_reqs() {
      // Serve the pending reads
      if (pending_reads.size()) {
        // Check the first pending request
        auto& req = pending_reads.front();
        if (req.depart <= m_clk) {
          // Request received data from dram
          if (req.depart - req.arrive > 1) {
            // Check if this requests accesses the DRAM or is being forwarded.
            // TODO add the stats back
            // s_read_latency += req.depart - req.arrive;
          }

          if (req.callback) {
            // If the request comes from outside (e.g., processor), call its callback
            std::cout << "[AiM Controller] callback request type: " << req.command << " addr: " << std::hex << req.addr << std::endl;
            req.callback(req);
          }
          // Finally, remove this request from the pending queue
          pending_reads.pop();
        }
      }
      
      // Serve the pending writes
      if (pending_writes.size()) {
        auto write_req_it = pending_writes.begin();
        while (write_req_it != pending_writes.end()) {
          if (write_req_it->depart <= m_clk) {
            if(write_req_it->callback){
              std::cout << "[AiM Controller] callback request type: " << write_req_it->command << " addr: " << std::hex << write_req_it->addr << std::endl;
              write_req_it->callback(*write_req_it);
            }
            write_req_it = pending_writes.erase(write_req_it); 
          }
          else { ++write_req_it; }
        }
      }
      
      // Serve the AiM bank commands
      if (pending_aim_bank.size()) {
        auto& req = pending_aim_bank.front();
        std::cerr << "[AiM Controller CH" << m_channel_id << " serve_completed_reqs()] pending_aim_bank: type=" << req.type_id
                  << " addr=0x" << std::hex << req.addr << std::dec
                  << " depart=" << req.depart << " m_clk=" << m_clk
                  << " has_callback=" << (req.callback ? 1 : 0) << std::endl;
        if (req.depart <= m_clk) {
          if (req.callback) {
            // If the request comes from outside (e.g., processor), call its callback
            std::cerr << "[AiM Controller CH" << m_channel_id << "] callback request type: " << req.command << " addr: " << std::hex << req.addr << std::dec << std::endl;

            req.callback(req);
          }
          // Finally, remove this request from the pending queue
          pending_aim_bank.pop();
        }
      }

      // Serve the AiM no-bank commands
      if (pending_aim_no_bank.size()) {
        auto& req = pending_aim_no_bank.front();
        if (req.depart <= m_clk) {
          if (req.callback) {
            // If the request comes from outside (e.g., processor), call its callback
            std::cerr << "[AiM Controller] callback request type: " << req.command << " addr: " << std::hex << req.addr << std::endl;
            req.callback(req);
          }
          // Finally, remove this request from the pending queue
          pending_aim_no_bank.pop();
        }
      }
    };

    /**
     * @brief    Checks if we need to switch to write mode
     * 
     */
    void set_write_mode() {
      if (!m_is_write_mode) {
        if ((m_write_buffer.size() > m_wr_high_watermark * m_write_buffer.max_size) || m_read_buffer.size() == 0) {
          m_is_write_mode = true;
        }
      } else {
        if ((m_write_buffer.size() < m_wr_low_watermark * m_write_buffer.max_size) && m_read_buffer.size() != 0) {
          m_is_write_mode = false;
        }
      }
    };

    /**
     * @brief    Helper function to find a request to schedule from the buffers.
     * @details
     * This function determine whether a request to serve exists.
     * It looks buffers up in priority order; active->priority->read/write/aim request.
     */
    bool schedule_request(ReqBuffer::iterator& req_it, ReqBuffer*& req_buffer) {
      bool request_found = false;
      // 2.1
      // First, check the active buffer to serve inflight requests in order to avoid useless ACTs.
      if (req_it = m_scheduler->get_best_request(m_active_buffer); req_it != m_active_buffer.end()) {
        if (m_dram->check_ready(req_it->command, req_it->addr_h)) {
          request_found = true;
          req_buffer = &m_active_buffer;
        }
      }

      // 2.2
      // Second, check the rest of the buffers since no request to schedule from the active buffer.
      if (!request_found) {
        // 2.2.1
        // Check the priority buffer
        if (m_priority_buffer.size() != 0) {
          req_it = m_priority_buffer.begin();
          req_it->command = m_dram->get_preq_command(req_it->final_command, req_it->addr_h);
          request_found = m_dram->check_ready(req_it->command, req_it->addr_h);
          req_buffer = &m_priority_buffer;
          if (!request_found && m_priority_buffer.size() != 0) {
            if (m_aim_bank_buffer.size() > 0 || m_aim_no_bank_buffer.size() > 0) {
              std::cerr << "[AiM Controller CH" << m_channel_id << " schedule_request()] BLOCKED by priority_buffer: "
                        << "priority_buf_size=" << m_priority_buffer.size()
                        << " priority_cmd=" << req_it->command
                        << " priority_type=" << req_it->type_id << std::endl;
            }
            return false;
          }
        }

        // 2.2.2
        // Check the AiM buffer first, and then read or write, since no request to schedule from the priority buffer.
        if (!request_found) {
          if (m_aim_bank_buffer.size() != 0) {
            // AiM does not require request scheduling (in-order execution),
            // thus, logic is the same as checking priority_buffer, not active, read, and write buffers.
            req_it = m_aim_bank_buffer.begin();
            req_it->command = m_dram->get_preq_command(req_it->final_command, req_it->addr_h);
            request_found = m_dram->check_ready(req_it->command, req_it->addr_h);
            if (!request_found && (m_aim_bank_buffer.size() > 0 || m_aim_no_bank_buffer.size() > 0)) {
              std::cerr << "[AiM Controller CH" << m_channel_id << " schedule_request()] AiM req blocked: "
                        << "preq_cmd=" << req_it->command << " final_cmd=" << req_it->final_command
                        << " type=" << req_it->type_id << " addr=0x" << std::hex << req_it->addr << std::dec
                        << " addr_h=[";
              for (size_t i = 0; i < req_it->addr_h.size(); i++) {
                std::cerr << req_it->addr_h[i];
                if (i < req_it->addr_h.size() - 1) std::cerr << ",";
              }
              std::cerr << "]" << std::endl;
            }
            req_buffer = &m_aim_bank_buffer;
          } else if (m_aim_no_bank_buffer.size() != 0) {
            req_it = m_aim_no_bank_buffer.begin();
            req_it->command = m_dram->get_preq_command(req_it->final_command, req_it->addr_h);
            request_found = m_dram->check_ready(req_it->command, req_it->addr_h);
            req_buffer = &m_aim_no_bank_buffer;
          } else {
            // Query the write policy to decide which buffer to serve
            set_write_mode();
            auto& buffer = m_is_write_mode ? m_write_buffer : m_read_buffer;
            if (req_it = m_scheduler->get_best_request(buffer); req_it != buffer.end()) {
              request_found = m_dram->check_ready(req_it->command, req_it->addr_h);
              req_buffer = &buffer;
            }
          }
        }
      }

      // 2.3 If we find a request to schedule, we need to check if it will close an opened row in the active buffer.
      if (request_found) {
        if (m_dram->m_command_meta(req_it->command).is_closing) {
          // If the command is PRE then check whether any request in the active buffer requires the row open,
          // by iteration.
          auto& rowgroup = req_it->addr_h;
          for (auto _it = m_active_buffer.begin(); _it != m_active_buffer.end(); _it++) {
            auto& _it_rowgroup = _it->addr_h;
            bool is_matching = true;
            for (int i = 0; i < m_bank_addr_idx + 1 ; i++) {
              if (_it_rowgroup[i] != rowgroup[i] && _it_rowgroup[i] != -1 && rowgroup[i] != -1) {
                is_matching = false;
                break;
              }
            }
            if (is_matching) {
              // The requst requires the opend row before closing.
              // Reverse the decision.
              request_found = false;
              break;
            }
          }
        }
      }

      return request_found;
    }

    void finalize() override {
      // s_avg_read_latency = (float) s_read_latency / (float) s_num_read_reqs;
      // s_queue_len_avg = (float) s_queue_len / (float) m_clk;
      // s_read_queue_len_avg = (float) s_read_queue_len / (float) m_clk;
      // s_write_queue_len_avg = (float) s_write_queue_len / (float) m_clk;
      // s_priority_queue_len_avg = (float) s_priority_queue_len / (float) m_clk;
      return;
    }

    void print_stats(YAML::Emitter& emitter) override {
      std::vector<std::string> command_name = {
        "ACT-1", "ACT-2", "PRE", "PREA", "CASRD", "CASWR",
        "RD16", "WR16", "RD16A", "WR16A", "REFab", "REFpb",
        "RFMab", "RFMpb", "MACSB", "AFSB", "RDCP", "WRCP",
        "ACT4_BG-1", "ACT4_BG-2", "PRE4_BG",
        "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD",
        "ACT16-1", "ACT16-2", "MACAB", "AFAB", "WRAFLUT", "WRBK",
        "WRGB", "WRMAC", "WRBIAS", "RDMAC", "RDAF"
      };

      std::vector<std::string> req_type_name = {
        "Read", "Write", "MAC_SBK", "AF_SBK", "COPY_BKGB", "COPY_GBBK",
        "MAC_4BK_INTRA_BG", "AF_4BK_INTRA_BG", "EWMUL", "EWADD",
        "MAC_ABK", "AF_ABK", "WR_AFLUT", "WR_BK",
        "WR_GB", "WR_MAC", "WR_BIAS", "RD_MAC", "RD_AF"
      };

      emitter << YAML::Key << get_ifce_name();
      emitter << YAML::Value;
      emitter << YAML::BeginMap;
      
      // Print my implementation name
      emitter << YAML::Key << "impl";
      emitter << YAML::Value << get_name();

      // Print my id if existent
      int id;
      if (get_id() != "_default_id") {
        emitter << YAML::Key << "id";
        emitter << YAML::Value << get_id();
        id = std::stoi(get_id().substr(get_id().find_last_of(' ') + 1));
      }

      // Commands
      emitter << YAML::Key << "Commands";
      emitter << YAML::Value;
      emitter << YAML::BeginMap;

      std::vector<std::string> ordered_keys_cmds;
      for (int j = 0; j < command_name.size(); j++) {
        
        std::string name = fmt::format("CH{}_num_{}_commands", id, command_name[j]);
        ordered_keys_cmds.push_back(name);
      }

      const auto& registry = m_stats.registry();

      for (const auto& key : ordered_keys_cmds) {
        auto it = registry.find(key);
        if (it != registry.end()) {
          it->second->emit_to(emitter);
        }
      }
      emitter << YAML::EndMap;

      // Cycles
      emitter << YAML::Key << "Cycles";
      emitter << YAML::Value;
      emitter << YAML::BeginMap;

      std::vector<std::string> ordered_keys_cycles;
      for (int j = 0; j < req_type_name.size(); j++) {
        std::string name = fmt::format("CH{}_AiM_{}_cycles", id, req_type_name[j]);
        ordered_keys_cycles.push_back(name);
      }
      ordered_keys_cycles.push_back(fmt::format("CH{}_idle_cycles", id));
      ordered_keys_cycles.push_back(fmt::format("CH{}_active_cycles", id));
      ordered_keys_cycles.push_back(fmt::format("CH{}_precharged_cycles", id));

      for (const auto& key : ordered_keys_cycles) {
        auto it = registry.find(key);
        if (it != registry.end()) {
          it->second->emit_to(emitter);
        }
      }

      emitter << YAML::EndMap;

      // Print all my children
      for (auto child_impl : m_children) {
        if (child_impl->has_stats()) {
          // TODO: Is this a bug in yaml-cpp that I have to emit NewLine twice?
          emitter << YAML::Newline;
          emitter << YAML::Newline;
        }
        child_impl->print_stats(emitter);
      }

      emitter << YAML::EndMap;
      emitter << YAML::Newline;
    }
};
  
}   // namespace Ramulator
