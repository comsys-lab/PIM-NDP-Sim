#ifndef RAMULATOR_ADDR_MAPPER_ADDR_MAPPER_H
#define RAMULATOR_ADDR_MAPPER_ADDR_MAPPER_H

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include "base/base.h"
#include "dram_controller/controller.h"
#include "base/AiM_request.h"

namespace Ramulator {

class IAddrMapper {
  RAMULATOR_REGISTER_INTERFACE(IAddrMapper, "AddrMapper", "Memory Controller Address Mapper");

  public:
    /**
     * @brief  Applies the address mapping to a physical address and returns the DRAM address vector
     * 
     */
    virtual void apply(Request& req) = 0;

    virtual std::vector<Addr_t> convert_pkt_addr(const Trace& trace) = 0;
    virtual IDRAM* get_m_dram() = 0;
};

}       // namespace Ramulator

#endif  // RAMULATOR_ADDR_MAPPER_ADDR_MAPPER_H