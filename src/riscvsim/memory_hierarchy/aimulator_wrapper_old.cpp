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
#include "aimulator_wrapper.h"
#include "memory_controller_utils.h"

#include <unistd.h>

aimulator_wrapper::aimulator_wrapper(std::string config_path)
    : config(Ramulator::Config::parse_config_file(config_path, {}))
    //   read_cb_func(std::bind(&aimulator_wrapper::read_complete, this, std::placeholders::_1)),
    //   write_cb_func(std::bind(&aimulator_wrapper::write_complete, this, std::placeholders::_1))
{
    ramulator2_frontend = Ramulator::Factory::create_frontend(config);
    ramulator2_memorysystem = Ramulator::Factory::create_memory_system(config);
    ramulator2_frontend->connect_memory_system(ramulator2_memorysystem);
    ramulator2_memorysystem->connect_frontend(ramulator2_frontend);
}

aimulator_wrapper::~aimulator_wrapper()
{
    // Ramulator::Logging::destroy_all_loggers();
    delete ramulator2_frontend;
    delete ramulator2_memorysystem;
}

void
aimulator_wrapper::finish()
{
    ramulator2_frontend->finalize();
    ramulator2_memorysystem->finalize();
}

void
aimulator_wrapper::finish_and_print_stats(const char* stats_dir, const char* timestamp)
{
    ramulator2_frontend->finalize_wrapper(stats_dir, timestamp);
    ramulator2_memorysystem->finalize_wrapper(stats_dir, timestamp);
}

// Deprecated in Ramulator 2
// void
// aimulator_wrapper::read_complete(Ramulator::Request &req)
// {
//     auto it = mem_addr_cb_status.find(req.addr);
//     assert(it != mem_addr_cb_status.end());
//     it->second = true;
// }

// void
// aimulator_wrapper::write_complete(Ramulator::Request &req)
// {
//     auto it = mem_addr_cb_status.find(req.addr);
//     assert(it != mem_addr_cb_status.end());
//     it->second = true;
// }

bool
aimulator_wrapper::access_complete()
{
    auto it = mem_addr_cb_status.begin();

    while (it != mem_addr_cb_status.end())
    {
        if (it->second == false)
        {
            return false;
        }
        it++;
    }

    return true;
}

bool
aimulator_wrapper::add_transaction(target_ulong addr, bool isWrite)
{
    // std::cout << "rd/wr transaction" << std::endl;
    int mem_access_type = 0;
    int src_id = 0;
    if (isWrite)
    {
        mem_access_type = 1;
    }

    bool request_sent = ramulator2_frontend->receive_external_requests(mem_access_type, addr, src_id,
        [this](Ramulator::Request& req) {
            auto it = mem_addr_cb_status.find(req.addr);
            assert(it != mem_addr_cb_status.end());
            it->second = true;
        });
    return request_sent;
}

bool
aimulator_wrapper::add_aim_transaction(target_ulong addr, int type_id)
{
    // std::cout << "aim_transaction" << std::endl;
    bool request_sent = false;
    request_sent = ramulator2_frontend->receive_external_aim_requests(type_id, addr,
        [this](Ramulator::Request& req) {
            auto it = mem_addr_cb_status.find(req.addr);
            assert(it != mem_addr_cb_status.end());
            it->second = true;
        });
    return request_sent;
}

int
aimulator_wrapper::get_max_clock_cycles(PendingMemAccessEntry *e)
{
    int bytes_accessed = 0;
    int clock_cycles_elasped = 0;
    target_ulong addr = e->addr;

    mem_addr_cb_status.clear();

    /* Split the entire request size into MEM_BUS_WIDTH sized parts, and query
     * latency for each of the part separately */
    if (e->type == MEM_ACCESS_READ || e->type == MEM_ACCESS_WRITE)
    {
        while (bytes_accessed <= e->access_size_bytes)
        {
            addr += bytes_accessed;
            mem_addr_cb_status.insert(std::pair<target_ulong, bool>(addr, false));
            assert(add_transaction(addr, (bool)e->type));
            bytes_accessed += MEM_BUS_WIDTH;
        }
    }
    else
    {
        mem_addr_cb_status.insert(std::pair<target_ulong, bool>(addr, false));
        assert(add_aim_transaction(addr, e->type));
    }

    /**
     * @todo\
     * - Handle if add_*_transaction fails.
     *   - Determine how to implement retry logic.
     *   - Determine how the retry logic affect the CPU simulation.
     *   - Implement when to tick() depending on the retry logic.
     */

    while (!access_complete())
    {
        ramulator2_memorysystem->tick();
        clock_cycles_elasped++;
    }

    return clock_cycles_elasped;
}
