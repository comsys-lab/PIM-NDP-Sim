#include <filesystem>
#include <iostream>
#include <fstream>

#include "frontend/frontend.h"
#include "base/AiM_request.h"
#include "base/exception.h"
#include "addr_mapper/addr_mapper.h"

namespace Ramulator {

namespace fs = std::filesystem;

class AiMPacketTrace : public IFrontEnd, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IFrontEnd, AiMPacketTrace, "AiMPacketTrace", "AiM memory address trace.")

  public:
    IDRAM* m_dram = nullptr;
    std::vector<int> m_addr_bits;
    Addr_t m_tx_offset = -1;

  public:
    void init() override {
      std::string trace_path_str = param<std::string>("path").desc("Path to the read write trace file.").required();
      m_clock_ratio = param<uint>("clock_ratio").required();
      m_addr_mapper = create_child_ifce<IAddrMapper>();
      m_logger = Logging::create_logger("AiMPacketTrace");
      m_logger->info("Loading trace file {} ...", trace_path_str);
      init_trace(trace_path_str);
      init_wrapper();
      m_logger->info("Loaded {} lines.", m_trace.size());
    };

    // AiMulator wrapper 1
    bool receive_external_requests(int req_type_id, Addr_t addr, int source_id, std::function<void(Request&)> callback) {
      Request req = Request(addr, req_type_id, source_id, callback);
      return m_memory_system->send(req);
    };

    // AiMulator wrapper 2
    bool receive_external_aim_requests(int req_type_id, Addr_t addr, std::function<void(Request&)> final_callback) {
      /* Q1: What should the addr include?
       * A1: addr = {ch_mask (16b), bank_mask (16b), row_addr (16b), col_addr (16b)}
       * Q2: Do we need to address mapper although the argument is in addr?
       */

      // Create trace entry from external request
      Trace tmp_tr = {};
      tmp_tr.type_id = req_type_id;
      
      // Determine number of banks based on request type
      switch (req_type_id) {
        case Request::Type::MAC_SBK:
        case Request::Type::AF_SBK:
        case Request::Type::COPY_BKGB:
        case Request::Type::COPY_GBBK:
          tmp_tr.aim_num_banks = 1;
          break;
        case Request::Type::MAC_4BK_INTRA_BG:
        case Request::Type::AF_4BK_INTRA_BG:
        case Request::Type::EWMUL:
        case Request::Type::EWADD:
          tmp_tr.aim_num_banks = 4;
          break;
        case Request::Type::MAC_ABK:
        case Request::Type::AF_ABK:
        case Request::Type::WR_AFLUT:
        case Request::Type::WR_BK:
          tmp_tr.aim_num_banks = 16;
          break;
        case Request::Type::WR_GB:
        case Request::Type::WR_MAC:
        case Request::Type::WR_BIAS:
        case Request::Type::RD_MAC:
        case Request::Type::RD_AF:
          tmp_tr.aim_num_banks = 0;
          break;
        default:
          tmp_tr.aim_num_banks = -1;
          break;
      }
      
      // Generate a mask
      uint64_t mask = (1U << m_addr_bits[0]) - 1;
      int ch_addr_lsb_pos = 0;
      for (int i = 1; i < m_dram->m_levels.size(); i++) {
        ch_addr_lsb_pos += m_addr_bits[i];
      }
      // Extract one-hot encoded channel from the given address
      uint64_t masked_ch = addr & (mask << ch_addr_lsb_pos);
      uint32_t one_hot_encoded_chs = masked_ch >> ch_addr_lsb_pos;
      // Convert the extracted channel to multiple channel addresses
      std::vector<uint16_t> ch_addrs;
      for (int bit_pos = 0; bit_pos < m_addr_bits[m_dram->m_levels("channel")]; bit_pos++) {
        if (one_hot_encoded_chs & (1U << bit_pos)) {
          ch_addrs.push_back(bit_pos);
        }
      }
      assert(!ch_addrs.empty());
      // Construct multiple addresses
      std::vector<Addr_t> addrs;
      for (int i = 0; i < ch_addrs.size(); i++) {
        addrs.push_back(static_cast<Addr_t>((ch_addrs[i] << ch_addr_lsb_pos) | (addr & ~mask)));
      }

      // Set is_aim flag based on number of banks
      switch (tmp_tr.aim_num_banks) {
        case -1:
          tmp_tr.is_aim = false;
          break;
        case 0:
        case 1:
        case 4:
        case 16:
          tmp_tr.is_aim = true;
          break;
        default:
          tmp_tr.is_aim = false;
          break;
      }

      /* From the 64-bit address including masks, multiple sub-requests can be geneated.
       * The generated sub-requests should not invoke the same callback function argument.
       * shared_ptr is for managing the lifetime of the counter for each sub-requests.
       * intermediate_callback lambda is for capturing the shared counter and the origianl, final_callback.
       */
      auto state = std::make_shared<int>(addrs.size());
      auto intermediate_callback = [state, final_callback](Request& req) {
        (*state)--;
        if (*state == 0) { final_callback(req); }
      };

      /* Create the vector of AiM requests, giving each one the same intermediate_callback.
       * They will all share the same counter.
       */
      std::vector<Request> aim_reqs;
      aim_reqs.reserve(addrs.size());
      for (const auto& aim_addr : addrs) {
        aim_reqs.emplace_back(tmp_tr.is_aim, tmp_tr.type_id, tmp_tr.aim_num_banks, intermediate_callback);
        aim_reqs.back().addr = aim_addr;
      }
      return m_memory_system->send(aim_reqs);
    };

    // AiMulator trace-based
    void tick() override {
      if (m_curr_trace_idx >= m_trace.size()) return;

      const Trace& t = m_trace[m_curr_trace_idx];
      bool request_sent = false;

      if (!t.is_aim) {
        Request rw_req = Request(t.addr, t.type_id);
        request_sent = m_memory_system->send(rw_req);
      } else {
        std::vector<Request> aim_reqs = convert_aim_pkt_trace_to_aim_reqs(t);
        request_sent = m_memory_system->send(aim_reqs);
      }

      if (request_sent) {
        m_curr_trace_idx = (m_curr_trace_idx + 1) % m_trace_length;
        m_trace_count++;
      }

      // TODO: Handle rejected requests assuming Instruction set register (ISR) FSM
      // Do we have to handle for each channel?
      // else {}
    };

    // TODO: FIXME
    bool is_finished() override {
      return m_trace_count >= m_trace_length;
    };

  private:
    IAddrMapper* m_addr_mapper = nullptr;
    std::vector<Trace> m_trace;
    
    size_t m_trace_length = 0;
    size_t m_curr_trace_idx = 0;
    size_t m_trace_count = 0;
    
    Logger_t m_logger;
  
  private:
    void init_trace(const std::string& file_path_str) {
      fs::path trace_path(file_path_str);
      if (!fs::exists(trace_path)) {
        throw ConfigurationError("Trace {} does not exist!", file_path_str);
      }
      std::ifstream trace_file(trace_path);
      if (!trace_file.is_open()) {
        throw ConfigurationError("Trace {} cannot be opened!", file_path_str);
      }

      std::string line;
      while (std::getline(trace_file, line)) {
        if (line[0] == '#' || line.empty()){
          continue; // comment or empty line
        }

        std::vector<std::string> tokens;
        tokenize(tokens, line, " ");
        Trace trace_entry {};

        if (tokens.empty()) continue;

        if (tokens[0] == "R") {
          trace_entry.type_id = Request::Type::Read;
          trace_entry.aim_num_banks = -1;
        } else if (tokens[0] == "W") {
          trace_entry.type_id = Request::Type::Write;
          trace_entry.aim_num_banks = -1;
        } else if (tokens[0] == "MAC_SBK") {
          trace_entry.type_id = Request::Type::MAC_SBK;
          trace_entry.aim_num_banks = 1;
        } else if (tokens[0] == "AF_SBK") {
          trace_entry.type_id = Request::Type::AF_SBK;
          trace_entry.aim_num_banks = 1;
        } else if (tokens[0] == "COPY_BKGB") {
          trace_entry.type_id = Request::Type::COPY_BKGB;
          trace_entry.aim_num_banks = 1;
        } else if (tokens[0] == "COPY_GBBK") {
          trace_entry.type_id = Request::Type::COPY_GBBK;
          trace_entry.aim_num_banks = 1;
        } else if (tokens[0] == "MAC_4BK_INTRA_BG") {
          trace_entry.type_id = Request::Type::MAC_4BK_INTRA_BG;
          trace_entry.aim_num_banks = 4;
        } else if (tokens[0] == "AF_4BK_INTRA_BG") {
          trace_entry.type_id = Request::Type::AF_4BK_INTRA_BG;
          trace_entry.aim_num_banks = 4;
        } else if (tokens[0] == "EWMUL") {
          trace_entry.type_id = Request::Type::EWMUL;
          trace_entry.aim_num_banks = 4;
        } else if (tokens[0] == "EWADD") {
          trace_entry.type_id = Request::Type::EWADD;
          trace_entry.aim_num_banks = 4;
        } else if (tokens[0] == "MAC_ABK") {
          trace_entry.type_id = Request::Type::MAC_ABK;
          trace_entry.aim_num_banks = 16;
        } else if (tokens[0] == "AF_ABK") {
          trace_entry.type_id = Request::Type::AF_ABK;
          trace_entry.aim_num_banks = 16;
        } else if (tokens[0] == "WR_AFLUT") {
          trace_entry.type_id = Request::Type::WR_AFLUT;
          trace_entry.aim_num_banks = 16;
        } else if (tokens[0] == "WR_BK") {
          trace_entry.type_id = Request::Type::WR_BK;
          trace_entry.aim_num_banks = 16;
        } else if (tokens[0] == "WR_GB") {
          trace_entry.type_id = Request::Type::WR_GB;
          trace_entry.aim_num_banks = 0;
        } else if (tokens[0] == "WR_MAC") {
          trace_entry.type_id = Request::Type::WR_MAC;
          trace_entry.aim_num_banks = 0;
        } else if (tokens[0] == "WR_BIAS") {
          trace_entry.type_id = Request::Type::WR_BIAS;
          trace_entry.aim_num_banks = 0;
        } else if (tokens[0] == "RD_MAC") {
          trace_entry.type_id = Request::Type::RD_MAC;
          trace_entry.aim_num_banks = 0;
        } else if (tokens[0] == "RD_AF") {
          trace_entry.type_id = Request::Type::RD_AF;
          trace_entry.aim_num_banks = 0;
        } else {
          throw ConfigurationError("Trace {} format invalid!", file_path_str);
        }
        // else if (tokens[0] == "MAC_4BK_INTER_BG") {
        //   trace_entry.type_id = Request::Type::MAC_4BK_INTER_BG;
        //   trace_entry.aim_num_banks = 4;
        // } else if (tokens[0] == "AF_4BK_INTER_BG") {
        //   trace_entry.type_id = Request::Type::AF_4BK_INTER_BG;
        //   trace_entry.aim_num_banks = 4;
        // } 
        
        switch (trace_entry.aim_num_banks) {
          case -1:
            trace_entry.is_aim = false;
            trace_entry.addr = std::stoll(tokens[1]);
            break;
          case 0:
            trace_entry.is_aim = true;
            trace_entry.ch_mask = static_cast<uint16_t>(std::stoi(tokens[1]));
            // trace_entry.reg_id = static_cast<int16_t>(std::stoi(tokens[2]));
            break;
          case 1:
            trace_entry.is_aim = true;
            trace_entry.ch_mask = static_cast<uint16_t>(std::stoi(tokens[1]));
            trace_entry.addr = std::stoll(tokens[7]);
            break;
          case 4:
            trace_entry.is_aim = true;
            trace_entry.ch_mask = static_cast<uint16_t>(std::stoi(tokens[1]));
            trace_entry.rank_addr = static_cast<uint16_t>(std::stoi(tokens[2]));
            trace_entry.pch_addr = static_cast<uint16_t>(std::stoi(tokens[3]));
            trace_entry.bank_addr_or_mask = static_cast<uint16_t>(std::stoi(tokens[4]));
            trace_entry.row_addr = static_cast<uint32_t>(std::stoi(tokens[5]));
            trace_entry.col_addr = static_cast<uint16_t>(std::stoi(tokens[6]));
            break;
          case 16:
            trace_entry.is_aim = true;
            trace_entry.ch_mask = static_cast<uint16_t>(std::stoi(tokens[1]));
            trace_entry.rank_addr = static_cast<uint16_t>(std::stoi(tokens[2]));
            trace_entry.pch_addr = static_cast<uint16_t>(std::stoi(tokens[3]));
            trace_entry.bank_addr_or_mask = static_cast<uint16_t>(std::stoi(tokens[4]));
            trace_entry.row_addr = static_cast<uint32_t>(std::stoi(tokens[5]));
            trace_entry.col_addr = static_cast<uint16_t>(std::stoi(tokens[6]));
            break;
          default: break;
        }

        m_trace.push_back(trace_entry);
      }

      trace_file.close();
      m_trace_length = m_trace.size();
    };

    void init_wrapper() {
      // Prepare address metadata of the DRAM hierarchy
      IDRAM* m_dram = m_memory_system->get_ifce<IDRAM>();
      const auto& count = m_dram->m_organization.count;
      m_addr_bits.resize(count.size());
      for (size_t level = 0; level < m_addr_bits.size(); level++) {
        m_addr_bits[level] = calc_log2(count[level]);
      }

      m_addr_bits[count.size() - 1] -= calc_log2(m_dram->m_internal_prefetch_size);
      int tx_bytes = m_dram->m_internal_prefetch_size * m_dram->m_channel_width / 8;
      m_tx_offset = calc_log2(tx_bytes);
    }

    std::vector<Request> convert_aim_pkt_trace_to_aim_reqs(const Trace& t) {
      // Convert addresses of the trace to that of the request.
      std::vector<Addr_t> aim_req_addrs = m_addr_mapper->convert_pkt_addr(t);
      // Initialize the request
      std::vector<Request> aim_reqs(aim_req_addrs.size(), Request(t.is_aim, t.type_id, t.aim_num_banks));
      // Copy aim_req and assign address for each
      for (int i = 0; i < aim_req_addrs.size(); i++) {
        aim_reqs[i].addr = aim_req_addrs[i];
      }
      return aim_reqs;
    };

};

}        // namespace Ramulator