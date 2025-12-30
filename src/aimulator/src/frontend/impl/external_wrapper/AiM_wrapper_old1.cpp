#include <filesystem>
#include <iostream>
#include <fstream>

#include "frontend/frontend.h"
#include "base/AiM_request.h"
#include "base/exception.h"
#include "addr_mapper/addr_mapper.h"

namespace Ramulator {

namespace fs = std::filesystem;

class AiMWrapper : public IFrontEnd, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IFrontEnd, AiMWrapper, "AiMWrapper", "AiM wrapper for another simulators.")

  public:
    void init() override {
      auto existing_logger = spdlog::get("AiMWrapper");
      // if (existing_logger) {
      //   m_logger = existing_logger;
      // } else {
      //   m_logger = Logging::create_logger("AiMWrapper");
      // }
      m_addr_mapper = create_child_ifce<IAddrMapper>();

      // Initialize address mapping information
      IDRAM* m_dram_wrapper = m_addr_mapper->get_m_dram();
      const auto& count = m_dram_wrapper->m_organization.count;
      m_addr_bits.resize(count.size());
      for (size_t level = 0; level < m_addr_bits.size(); level++) {
        m_addr_bits[level] = calc_log2(count[level]);
      }

      m_addr_bits[count.size() - 1] -= calc_log2(m_dram_wrapper->m_internal_prefetch_size);
      int tx_bytes = m_dram_wrapper->m_internal_prefetch_size * m_dram_wrapper->m_channel_width / 8;
      m_tx_offset = calc_log2(tx_bytes);

      // Calculate channel position and mask
      m_ch_addr_lsb_pos = 0;
      for (int i = 1; i < m_dram_wrapper->m_levels.size(); i++) {
        m_ch_addr_lsb_pos += m_addr_bits[i];
      }
      m_ch_mask = ((1ULL << m_addr_bits[0]) - 1) << m_ch_addr_lsb_pos;
      
      // Get number of channels for max batch size
      m_num_chs = count[m_dram_wrapper->m_levels("channel")];
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

      /*  DEPRECATED
      // Generate a mask
      // uint64_t mask = (1U << m_addr_bits[0]) - 1;
      // int ch_addr_lsb_pos = 0;
      // for (int i = 1; i < m_dram_wrapper->m_levels.size(); i++) {
      //   ch_addr_lsb_pos += m_addr_bits[i];
      // }
      // // 2. Extract one-hot encoded channel from the given address
      // uint64_t masked_ch = addr & (mask << ch_addr_lsb_pos);
      // uint32_t one_hot_encoded_chs = masked_ch >> ch_addr_lsb_pos;
      // // 3. Convert the extracted channel to multiple channel addresses
      // std::vector<uint16_t> ch_addrs;
      // for (int bit_pos = 0; bit_pos < m_addr_bits[m_dram_wrapper->m_levels("channel")]; bit_pos++) {
      //   if (one_hot_encoded_chs & (1U << bit_pos)) {
      //     ch_addrs.push_back(bit_pos);
      //   }
      // }
      // assert(!ch_addrs.empty());
      // // 4. Construct multiple addresses
      // std::vector<Addr_t> addrs;
      // for (int i = 0; i < ch_addrs.size(); i++) {
      //   addrs.push_back(static_cast<Addr_t>((ch_addrs[i] << ch_addr_lsb_pos) | (addr & ~mask)));
      // }
      */

      // 5. Set is_aim flag based on number of banks
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

      // Calculate address without channel bits for comparison
      Addr_t addr_without_ch = addr & ~m_ch_mask;

      // Check if this request can be batched with pending requests
      auto it = m_pending_batches.find(addr_without_ch);

      if (it == m_pending_batches.end()) {
        // Create new batch entry
        BatchInfo batch_info;
        batch_info.type_id = req_type_id;
        batch_info.aim_num_banks = tmp_tr.aim_num_banks;
        batch_info.is_aim = tmp_tr.is_aim;
        batch_info.addresses.push_back(addr);
        batch_info.callbacks.push_back(final_callback);
        m_pending_batches[addr_without_ch] = batch_info;
      } else {
        // Add to existing batch
        it->second.addresses.push_back(addr);
        it->second.callbacks.push_back(final_callback);
      }

      // Check if batch is ready to send (reached max size or should flush)
      auto& batch_info = m_pending_batches[addr_without_ch];
      if (batch_info.addresses.size() >= m_num_chs) {
        return flush_batch(addr_without_ch);
      }

      return true;

      /* DEPRECATED
      // From the 64-bit address including masks, multiple sub-requests can be geneated.
      // The generated sub-requests should not invoke the same callback function argument.
      // shared_ptr is for managing the lifetime of the counter for each sub-requests.
      // intermediate_callback lambda is for capturing the shared counter and the origianl, final_callback.
      
      // auto state = std::make_shared<int>(addrs.size());
      // auto intermediate_callback = [state, final_callback](Request& req) {
      //   (*state)--;
      //   if (*state == 0) { final_callback(req); }
      // };

      // Create the vector of AiM requests, giving each one the same intermediate_callback.
      // They will all share the same counter.
      // std::vector<Request> aim_reqs;
      // aim_reqs.reserve(addrs.size());
      // for (const auto& aim_addr : addrs) {
      //   aim_reqs.emplace_back(tmp_tr.is_aim, tmp_tr.type_id, tmp_tr.aim_num_banks, intermediate_callback);
      //   aim_reqs.back().addr = aim_addr;
      // }
      // return m_memory_system->send(aim_reqs);*/
    };

    void tick() override {};

  private:
    Logger_t m_logger;
    IAddrMapper* m_addr_mapper = nullptr;
    std::vector<int> m_addr_bits;
    Addr_t m_tx_offset = -1;

    Addr_t m_ch_mask = 0;
    int m_ch_addr_lsb_pos = 0;
    size_t m_num_chs = 0;

    struct BatchInfo {
      int type_id;
      int aim_num_banks;
      bool is_aim;
      std::vector<Addr_t> addresses;
      std::vector<std::function<void(Request&)>> callbacks;
    };
    std::unordered_map<Addr_t, BatchInfo> m_pending_batches;
  
  private:
    bool is_finished() override { return true; };

    // Flush a specific batch
    bool flush_batch(Addr_t addr_without_ch) {
      auto it = m_pending_batches.find(addr_without_ch);
      if (it == m_pending_batches.end()) {
        return false;
      }

      BatchInfo& batch_info = it->second;
      size_t batch_size = batch_info.addresses.size();

      // Create shared counter for all requests in this batch
      auto state = std::make_shared<std::vector<bool>>(batch_size, false);
      auto callbacks_copy = batch_info.callbacks;
      
      auto intermediate_callback = [state, callbacks_copy, batch_size](Request& req) {
        // Mark this request as completed
        for (size_t i = 0; i < batch_size; i++) {
          if (!(*state)[i]) {
            (*state)[i] = true;
            callbacks_copy[i](req);
            break;
          }
        }
      };

      // Create requests for each address in the batch
      std::vector<Request> aim_reqs;
      aim_reqs.reserve(batch_size);
      for (const auto& batch_addr : batch_info.addresses) {
        aim_reqs.emplace_back(batch_info.is_aim, batch_info.type_id, batch_info.aim_num_banks, intermediate_callback);
        aim_reqs.back().addr = batch_addr;
      }

      // Send the batch
      bool success = m_memory_system->send(aim_reqs);

      // Remove the batch from pending map
      m_pending_batches.erase(it);

      return success;
    };
    
    // Flush all pending batches (call this periodically or when needed)
    void flush_all_batches() {
      std::vector<Addr_t> keys;
      for (const auto& pair : m_pending_batches) {
        keys.push_back(pair.first);
      }
      for (const auto& key : keys) {
        flush_batch(key);
      }
    };
};

}        // namespace Ramulator