#ifndef     RAMULATOR_AIM_REQUEST_H
#define     RAMULATOR_AIM_REQUEST_H

#include <vector>
#include <list>
#include <string>

#include "base/base.h"

namespace Ramulator {

struct Request { 
  Addr_t    addr = -1;
  AddrHierarchy_t addr_h {};

  bool is_aim_req = false;

  // Basic request id convention
  // 0 = Read, 1 = Write. The device spec defines all others
  struct Type {
    enum : int {
      Read = 0, Write = 1,
      // AiM Single Bank
      MAC_SBK = 2, AF_SBK = 3, COPY_BKGB = 4, COPY_GBBK = 5,
      // AiM Multi-Bank
      MAC_4BK_INTRA_BG = 6, AF_4BK_INTRA_BG = 7, EWMUL = 8, EWADD = 9,
      MAC_ABK = 10, AF_ABK = 11, WR_AFLUT = 12, WR_BK = 13,
      // AiM No Bank
      WR_GB = 14, WR_MAC = 15, WR_BIAS = 16, RD_MAC = 17, RD_AF = 18,
      // End of Type
      UNKNOWN = 19,
      // TODO
      // MAC_4BK_INTER_BG = 8, AF_4BK_INTER_BG = 9,
    };
  };

  int aim_num_banks = -1;
  bool is_inter_bg = false;
  uint16_t inter_bg_bank_addrs;

  // An identifier for the type of the request
  int type_id = Request::Type::UNKNOWN;
  // An identifier for where the request is coming from (e.g., which core)
  int source_id = -1;

  // The command that need to be issued to progress the request
  int command = -1;
  // The final command that is needed to finish the request
  int final_command = -1;
  // Memory controller stats
  bool is_stat_updated = false;

  // Clock cycle when the request arrive at the memory controller
  Clk_t arrive = -1;
  // Clock cycle when the request depart the memory controller
  Clk_t depart = -1;
  // Clock cycle when the request issued at the memory controller
  Clk_t issue = -1;

  // A scratchpad for the request
  std::array<int, 4> scratchpad = { 0 };

  std::function<void(Request&)> callback;

  // Point to a generic payload
  void* m_payload = nullptr;

  Request(Addr_t addr, int type_id);
  Request(AddrHierarchy_t addr_h, int type_id);
  Request(Addr_t addr, int type_id, int source_id, std::function<void(Request&)> callback);

  // AiM
  // Trace-based
  Request(bool is_aim_req, int type_id, int aim_num_banks);
  // Wrapper
  Request(bool is_aim_req, int type_id, int aim_num_banks, std::function<void(Request&)> tmp_callback);
  // New Wrapper
  Request(bool is_aim_req, int req_type_id, Addr_t addr, int aim_num_banks, std::function<void(Request&)> callback);
};

inline std::string str_type_name(int type_id) {
  switch (type_id) {
    case Request::Type::Read: return "Read";
    case Request::Type::Write: return "Write";
    case Request::Type::MAC_SBK: return "MAC_SBK";
    case Request::Type::AF_SBK: return "AF_SBK";
    case Request::Type::COPY_BKGB: return "COPY_BKGB";
    case Request::Type::COPY_GBBK: return "COPY_GBBK";
    case Request::Type::MAC_4BK_INTRA_BG: return "MAC_4BK_INTRA_BG";
    case Request::Type::AF_4BK_INTRA_BG: return "AF_4BK_INTRA_BG";
    case Request::Type::EWMUL: return "EWMUL";
    case Request::Type::EWADD: return "EWADD";
    case Request::Type::MAC_ABK: return "MAC_ABK";
    case Request::Type::AF_ABK: return "AF_ABK";
    case Request::Type::WR_AFLUT: return "WR_AFLUT";
    case Request::Type::WR_BK: return "WR_BK";
    case Request::Type::WR_GB: return "WR_GB";
    case Request::Type::WR_MAC: return "WR_MAC";
    case Request::Type::WR_BIAS: return "WR_BIAS";
    case Request::Type::RD_MAC: return "RD_MAC";
    case Request::Type::RD_AF: return "RD_AF";
    default: return "UNKNOWN";
    // case Request::Type::MAC_4BK_INTER_BG: return "MAC_4BK_INTER_BG";
    // case Request::Type::AF_4BK_INTER_BG: return "AF_4BK_INTER_BG";
  }
};

struct ReqBuffer {
  std::list<Request> buffer;
  size_t max_size = 16384;

  using iterator = std::list<Request>::iterator;
  iterator begin() { return buffer.begin(); };
  iterator end() { return buffer.end(); };

  size_t size() const { return buffer.size(); }

  bool enqueue(const Request& request) {
    if (buffer.size() <= max_size) {
      buffer.push_back(request);
      return true;
    } else {
      return false;
    }
  }

  void remove(iterator it) {
    buffer.erase(it);
  }
};

struct Trace {
  // RW
  int type_id = -1;
  Addr_t addr = -1;
  // AiM
  bool is_aim = false;
  uint16_t ch_mask = 0;
  uint16_t rank_addr = 0;
  uint16_t pch_addr = 0;
  uint16_t bank_addr_or_mask = 0;
  uint32_t row_addr = 0;
  uint16_t col_addr = 0;
  int aim_num_banks = -1;
  bool is_inter_bg = false;
  // Are they essential?
  // int reg_id = -1;
  // int8_t rank_addr = -1;

  Trace() = default;
};

}        // namespace Ramulator


#endif   // RAMULATOR_AIM_REQUEST_H