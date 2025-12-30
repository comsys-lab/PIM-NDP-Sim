#include <vector>

#include "base/base.h"
#include "addr_mapper/addr_mapper.h"
#include "memory_system/memory_system.h"
#include "frontend/frontend.h"

namespace Ramulator {

class LinearMapperBase : public IAddrMapper {
  public:
    IDRAM* m_dram = nullptr;
    // How many levels in the hierarchy?
    int m_num_levels = -1;
    // How many address bits for each level in the hierarchy?
    std::vector<int> m_addr_bits;
    Addr_t m_tx_offset = -1;
    int m_col_bits_idx = -1;
    int m_row_bits_idx = -1;

  protected:
    void setup(IFrontEnd* frontend, IMemorySystem* memory_system) {
      m_dram = memory_system->get_ifce<IDRAM>();

      // Populate m_addr_bits vector with the number of address bits for each level in the hierachy
      const auto& count = m_dram->m_organization.count;
      m_num_levels = count.size();
      m_addr_bits.resize(m_num_levels);
      for (size_t level = 0; level < m_addr_bits.size(); level++) {
        m_addr_bits[level] = calc_log2(count[level]);
      }

      // Last (Column) address have the granularity of the prefetch size
      m_addr_bits[m_num_levels - 1] -= calc_log2(m_dram->m_internal_prefetch_size);

      int tx_bytes = m_dram->m_internal_prefetch_size * m_dram->m_channel_width / 8;
      m_tx_offset = calc_log2(tx_bytes);

      // Determine where are the row and col bits for ChRaBaRoCo and RoBaRaCoCh
      try {
        m_row_bits_idx = m_dram->m_levels("row");
      } catch (const std::out_of_range& r) {
        throw std::runtime_error(fmt::format("Organization \"row\" not found in the spec, cannot use linear mapping!"));
      }

      // Assume column is always the last level
      m_col_bits_idx = m_num_levels - 1;
    }

    std::vector<int> convert_mask_to_id(Addr_t mask, int level) {
      std::vector<int> addrs;
      int bitwidth = 1U << (m_addr_bits[level]);
      if (level == m_dram->m_levels("bank")) {
        bitwidth = 1U << (m_addr_bits[m_dram->m_levels("bankgroup")] + m_addr_bits[m_dram->m_levels("bank")]);
      }
      for (int bit_pos = 0; bit_pos < bitwidth; bit_pos++) {
        if (mask & (1U << bit_pos)) {
          addrs.push_back(bit_pos);
        }
      }
      return addrs;
    }
};

class ChRaBaRoCo final : public LinearMapperBase, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IAddrMapper, ChRaBaRoCo, "ChRaBaRoCo", "Applies a trival mapping to the address.");

  public:
    void init() override { };

    void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
      LinearMapperBase::setup(frontend, memory_system);
    }

    void apply(Request& req) override {
      req.addr_h.resize(m_num_levels, -1);
      Addr_t addr = req.addr >> m_tx_offset;
      for (int i = m_addr_bits.size() - 1; i >= 0; i--) {
        req.addr_h[i] = slice_lower_bits(addr, m_addr_bits[i]);
      }
    }

    std::vector<Addr_t> convert_pkt_addr(const Trace& trace) override {
      std::unordered_map<std::string_view, int> level_mem_hierarchy;
      for (int i = 0; i < m_dram->m_levels.size(); i++) {
        std::string_view level_name = m_dram->m_levels(i);
        if (m_dram->m_levels.contains(level_name)) {
          level_mem_hierarchy[level_name] = m_dram->m_levels(level_name);
        }
      }
      std::vector<int> ch_addrs = convert_mask_to_id(trace.ch_mask, level_mem_hierarchy["channel"]);
      // uint16_t flat_bank_addrs = 0;
      // std::vector<int> bank_addrs = convert_mask_to_id(trace.bank_addr_or_mask, level_mem_hierarchy["bank"]);
      // TODO: Handle inter-bankgroup AiM cmds.
      // if (trace.is_inter_bg) {
      //   // Inter-bank AiM; for 4 bankgroups/channel * 4 banks/bankgroup
      //   for (int i = 0; i < 4; i++) {
      //     flat_bank_addrs |= bank_addrs[i];
      //     flat_bank_addrs <<= 4;
      //   }
      // }

      // Map the converted addresses to the single address
      std::vector<Addr_t> addrs(ch_addrs.size(), 0);
      for (size_t i = 0; i < ch_addrs.size(); i++) {
        Addr_t addr = 0;

        // Build address from MSB to LSB with proper masking:
        //   Channel -> Rank -> Bank -> Row -> Column
        // Channel
        addr = ch_addrs[i] & ((1ULL << m_addr_bits[m_dram->m_levels("channel")]) - 1);
        // Rank is optional
        if (m_dram->m_levels.contains("rank") && trace.rank_addr != -1) {
          addr <<= m_addr_bits[level_mem_hierarchy["rank"]];
          uint16_t rank_mask = (1U << (m_addr_bits[level_mem_hierarchy["rank"]])) - 1;
          addr |= (trace.rank_addr & rank_mask);
        }
        // Pseudochannel is optional
        if (m_dram->m_levels.contains("pseudochannel")) {
          addr <<= m_addr_bits[level_mem_hierarchy["pseudochannel"]];
          uint16_t pch_mask = (1U << (m_addr_bits[level_mem_hierarchy["pseudochannel"]])) - 1;
          addr |= (trace.pch_addr & pch_mask);
        }
        // Bankgroup + Bank
        addr <<= m_addr_bits[level_mem_hierarchy["bankgroup"]] + m_addr_bits[level_mem_hierarchy["bank"]];
        uint16_t bank_mask = (1U << (m_addr_bits[level_mem_hierarchy["bankgroup"]] + m_addr_bits[level_mem_hierarchy["bank"]])) - 1;
        // TODO: Handle inter-bankgroup AiM cmds.
        // if (trace.is_inter_bg) {
        //   // 61440(=0xf000) for masking bank_addrs[0]
        //   bank_mask = 61440U;
        // } else {
        // }
        addr |= (trace.bank_addr_or_mask & bank_mask);
        // Row
        addr <<= m_addr_bits[m_row_bits_idx];
        uint32_t row_mask = (1U << m_addr_bits[m_row_bits_idx]) - 1;
        addr |= (trace.row_addr & row_mask);
        // Col
        addr <<= m_addr_bits[m_col_bits_idx];
        uint16_t col_mask = (1U << m_addr_bits[m_col_bits_idx]) - 1;
        addr |= (trace.col_addr & col_mask);

        addr <<= m_tx_offset;
        addrs[i] = addr;

        /* TODO
         * - Mask bank_addrs to addrs (return value) to assign
         *   Request.integer_bg_bank_addrs for INTER_BG AiM cmds.
         */
        // if (trace.is_inter_bg) {
        //   addrs[i]
        // }
      }
      return addrs;
    }

    IDRAM* get_m_dram() override {
      return m_dram;
    }
};

class RoBaRaCoCh final : public LinearMapperBase, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IAddrMapper, RoBaRaCoCh, "RoBaRaCoCh", "Applies a RoBaRaCoCh mapping to the address.");

  public:
    void init() override { };

    void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
      LinearMapperBase::setup(frontend, memory_system);
    }

    void apply(Request& req) override {
      req.addr_h.resize(m_num_levels, -1);
      Addr_t addr = req.addr >> m_tx_offset;
      req.addr_h[0] = slice_lower_bits(addr, m_addr_bits[0]);
      req.addr_h[m_addr_bits.size() - 1] = slice_lower_bits(addr, m_addr_bits[m_addr_bits.size() - 1]);
      for (int i = 1; i <= m_row_bits_idx; i++) {
        req.addr_h[i] = slice_lower_bits(addr, m_addr_bits[i]);
      }
    }

    std::vector<Addr_t> convert_pkt_addr(const Trace& trace) override {
      // For RoBaRaCoCh mapping, implement similar logic to ChRaBaRoCo
      // but with different address bit ordering
      std::vector<int> ch_addrs = convert_mask_to_id(trace.ch_mask, 0);
      std::vector<Addr_t> addrs(ch_addrs.size(), 0);

      // TODO: Implement RoBaRaCoCh-specific address conversion logic
      // This is a placeholder implementation - you may need to adjust
      // based on your specific RoBaRaCoCh mapping requirements
      
      for (size_t i = 0; i < ch_addrs.size(); i++) {
        Addr_t addr = 0;
        
        // RoBaRaCoCh: Row -> Bank -> Rank -> Column -> Channel
        // Build address with different bit ordering than ChRaBaRoCo
        addr = trace.row_addr;
        addr <<= m_addr_bits[m_dram->m_levels("bank")];
        addr |= trace.bank_addr_or_mask;  // Simplified - may need refinement
        addr <<= m_addr_bits[m_dram->m_levels("bankgroup")];
        addr <<= m_addr_bits[m_col_bits_idx];
        addr |= trace.col_addr;
        addr <<= m_addr_bits[0];  // Channel bits
        addr |= ch_addrs[i];
        addr <<= m_tx_offset;

        addrs[i] = addr;
      }
      return addrs;
    }

    IDRAM* get_m_dram() override {
      return m_dram;
    }
};

class MOP4CLXOR final : public LinearMapperBase, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IAddrMapper, MOP4CLXOR, "MOP4CLXOR", "Applies a MOP4CLXOR mapping to the address.");

  public:
    void init() override { };

    void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
      LinearMapperBase::setup(frontend, memory_system);
    }

    void apply(Request& req) override {
      req.addr_h.resize(m_num_levels, -1);
      Addr_t addr = req.addr >> m_tx_offset;
      req.addr_h[m_col_bits_idx] = slice_lower_bits(addr, 2);
      for (int lvl = 0 ; lvl < m_row_bits_idx ; lvl++)
          req.addr_h[lvl] = slice_lower_bits(addr, m_addr_bits[lvl]);
      req.addr_h[m_col_bits_idx] += slice_lower_bits(addr, m_addr_bits[m_col_bits_idx]-2) << 2;
      req.addr_h[m_row_bits_idx] = (int) addr;

      int row_xor_index = 0; 
      for (int lvl = 0 ; lvl < m_col_bits_idx ; lvl++){
        if (m_addr_bits[lvl] > 0){
          int mask = (req.addr_h[m_col_bits_idx] >> row_xor_index) & ((1<<m_addr_bits[lvl])-1);
          req.addr_h[lvl] = req.addr_h[lvl] xor mask;
          row_xor_index += m_addr_bits[lvl];
        }
      }
    }

    std::vector<Addr_t> convert_pkt_addr(const Trace& trace) override {
      // For MOP4CLXOR mapping, implement XOR-based address conversion
      std::vector<int> ch_addrs = convert_mask_to_id(trace.ch_mask, 0);
      std::vector<Addr_t> addrs(ch_addrs.size(), 0);

      // TODO: Implement MOP4CLXOR-specific address conversion logic
      // This is a placeholder implementation - you may need to adjust
      // based on your specific MOP4CLXOR mapping requirements
      
      for (size_t i = 0; i < ch_addrs.size(); i++) {
        Addr_t addr = 0;
        
        // Apply XOR-based mapping similar to the apply() method
        // Build basic address first
        addr = ch_addrs[i];
        addr <<= m_addr_bits[m_row_bits_idx];
        addr |= trace.row_addr;
        addr <<= m_addr_bits[m_col_bits_idx];
        addr |= trace.col_addr;
        addr <<= m_tx_offset;

        addrs[i] = addr;
      }
      return addrs;
    }

    IDRAM* get_m_dram() override {
      return m_dram;
    }
};
}   // namespace Ramulator