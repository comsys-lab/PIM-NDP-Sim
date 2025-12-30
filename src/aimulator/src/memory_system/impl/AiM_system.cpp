#include "memory_system/memory_system.h"
#include "translation/translation.h"
#include "dram_controller/controller.h"
#include "addr_mapper/addr_mapper.h"
#include "dram/AiM_dram.h"

namespace Ramulator {

class AiMSystem final : public IMemorySystem, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IMemorySystem, AiMSystem, "AiMSystem", "An AiM-based memory system.");

  public:
    void init() override {
      // Create device (a top-level node wrapping all channel nodes)
      m_dram = create_child_ifce<IDRAM>();
      m_addr_mapper = create_child_ifce<IAddrMapper>();
      num_chs = m_dram->get_level_size("channel");
      s_num_reqs = std::vector<std::vector<int>>(num_chs, std::vector<int>(Request::Type::UNKNOWN, 0));
      // callback = std::bind(&AiMDRAMSystem::receive, this, std::placeholders::_1);
      
      // Create memory controllers
      for (int i = 0; i < num_chs; i++) {
        IDRAMController* controller = create_child_ifce<IDRAMController>();
        controller->m_impl->set_id(fmt::format("Channel {}", i));
        controller->m_channel_id = i;
        m_controllers.push_back(controller);
        // for (int req_type = 0; req_type < Request::Type::UNKNOWN; req_type++) {
        //   register_stat(s_num_reqs[i][req_type])
        //     .name("CH_{}_total_num_{}_requests", i, str_type_name(req_type));
        // }
      }
    };

    bool send(Request req) override {
      m_addr_mapper->apply(req);
      int ch_id = req.addr_h[0];
      // std::cout << "[Ramulator (AiM Memory System)] ch_id: " << ch_id << std::endl;
      bool is_success = m_controllers[ch_id]->send(req);
      s_num_reqs[ch_id][req.type_id]++;
      // std::cout << "[Ramulator (AiM Memory System)] is_succes? " << is_success << std::endl;
      // for (int i = 0; i < num_chs; i++) {
      //   if (is_success) { s_num_reqs[i][req.type_id]++; }
      // }
      return is_success;
    };

    bool send(std::vector<Request> req_vec) override {
      // send requests within the req_vec to each channel
      std::vector<bool> is_success(req_vec.size(), false);
      bool is_all_success = true;
      for (int i = 0; i < req_vec.size(); i++) {
        m_addr_mapper->apply(req_vec[i]);
        int ch_id = req_vec[i].addr_h[0];
        is_success[i] = m_controllers[ch_id]->send(req_vec[i]);
        if (is_success[i]) { s_num_reqs[ch_id][req_vec[i].type_id]++; }
        /**
         * @todo
         * - Handle if the memory controller queue is full.
         */
        // else {
        //   aim_reqs_waiting_queue =
        // }
        is_all_success &= is_success[i];
      }
      return is_all_success;
    };
    
    void tick() override {
      m_clk++;
      m_dram->tick();
      for (auto controller : m_controllers) {
        controller->tick();
      }
    };

    float get_tCK() override {
      return m_dram->m_timing_vals("tCK_ps") / 1000.0f;
    }

    void print_stats(YAML::Emitter& emitter) override {
      std::string req_type_name[] = {
        "Read", "Write", "MAC_SBK", "AF_SBK", "COPY_BKGB", "COPY_GBBK",
        "MAC_4BK_INTRA_BG", "AF_4BK_INTRA_BG", "EWMUL", "EWADD",
        "MAC_ABK", "AF_ABK", "WR_AFLUT", "WR_BK",
        "WR_GB", "WR_MAC", "WR_BIAS", "RD_MAC", "RD_AF"
      };

      emitter << YAML::Key << "AiMSystem_Stats";
      emitter << YAML::Value << YAML::BeginMap;

      for (int i = 0; i < num_chs; i++) {
        emitter << YAML::Key << fmt::format("Channel_{}_Requests", i);
        emitter << YAML::Value << YAML::BeginMap;

        std::vector<std::string> ordered_keys;
        for (int j = 0; j < Request::Type::UNKNOWN; j++) {
          std::string name = fmt::format("CH_{}_total_num_{}_requests", i, req_type_name[j]);
          ordered_keys.push_back(name);
        }
        const auto& registry = m_stats.registry();

        for (const auto& key : ordered_keys) {
          auto it = registry.find(key);
          if (it != registry.end()) {
            it->second->emit_to(emitter);
          }
        }

        emitter << YAML::EndMap;
      }

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
    
    protected:
    Clk_t m_clk = 0;
    IDRAM* m_dram;
    uint16_t num_chs;
    IAddrMapper* m_addr_mapper;
    std::vector<IDRAMController*> m_controllers;
    Logger_t m_logger;
    std::queue<Request> request_queue;
    std::queue<Request> aim_reqs_waiting_queue;
    std::vector<std::vector<int>> s_num_reqs;
    int AiM_req_id = 0;
    int stalled_AiM_requests = 0;
    std::function<void(Request &)> callback;

    // const SpecDef& get_supported_requests() override {
    //   return m_dram->m_requests;
    // };
};
}   // namespace Ramulator
