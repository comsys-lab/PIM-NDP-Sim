/**
 * Ramulator CPP wrapper
 *
 * MARSS-RISCV : Micro-Architectural System Simulator for RISC-V
 *
 * Copyright (c) 2020 Gaurav Kothari {gkothar1@binghamton.edu}
 * State University of New York at Binghamton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef _RAMULATOR_WRAPPER_H_
#define _RAMULATOR_WRAPPER_H_

#include "../riscv_sim_typedefs.h"
#include "memory_controller_utils.h"
// AiM
#include "../../aimulator/src/base/base.h"
#include "../../aimulator/src/base/AiM_request.h"
#include "../../aimulator/src/base/config.h"
#include "../../aimulator/src/frontend/frontend.h"
#include "../../aimulator/src/memory_system/memory_system.h"

#include <fstream>
#include "../../aimulator/src/base/stats.h"

class aimulator_wrapper
{
  public:
    aimulator_wrapper(std::string config_path);
    ~aimulator_wrapper();

    // bool add_transaction(target_ulong addr, bool isWrite);
    bool add_aim_transaction(target_ulong addr, int type_id);
    int get_max_clock_cycles(PendingMemAccessEntry *e);
    bool access_complete();
    // void write_complete(Ramulator::Request &req);
    // void read_complete(Ramulator::Request &req);
    void finish();
    void finish_and_print_stats(const char* stats_dir, const char* timestamp);

  public:
    YAML::Node config;
    Ramulator::IFrontEnd* ramulator2_frontend;
    Ramulator::IMemorySystem* ramulator2_memorysystem;

    /* We split the cache-line address into MEM_BUS_WIDTH sized parts, so this map
     * keeps track of callbacks for each of this part */
    std::multimap<target_ulong, bool> mem_addr_cb_status;

    // Deprecated in Ramulator 2
    // std::function<void(Ramulator::Request&)> read_cb_func;
    // std::function<void(Ramulator::Request&)> write_cb_func;

  private:
    struct PendingTransaction {
      target_ulong addr;
      int type_id;
      bool is_aim;

      PendingTransaction(target_ulong addr, int type_id = -1, bool aim = false)
        : addr(addr), type_id(type_id), is_aim(aim) {}
    };

    std::queue<PendingTransaction> retry_queue;
    bool try_send_transaction(const PendingTransaction& trans);
};
#endif