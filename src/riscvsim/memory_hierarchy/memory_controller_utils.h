/**
 * Memory controller utilities
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
#ifndef _MEM_CONTROLLER_UTILS_H_
#define _MEM_CONTROLLER_UTILS_H_

#include "../riscv_sim_typedefs.h"

#define MEM_BUS_WIDTH 64               /* bytes */
#define LLC_TO_MEM_CONTROLLER_DELAY 3 /* cycles, one-way delay */

/* Memory operation type */
typedef enum MemAccessType {
    MEM_ACCESS_READ = 0x0,
    MEM_ACCESS_WRITE = 0x1,

    // AiM
    // STORE_MASK = 0x23
    AIM_MAC_SBK = 0x2,          // funct3 = 100
    AIM_AF_SBK = 0x3,           // funct3 = 101
    AIM_COPY_BKGB = 0x4,        // funct3 = 110
    AIM_COPY_GBBK = 0x5,        // funct3 = 111
    // AIM_CUSTOM_1_MASK = 0x2b
    AIM_MAC_4BK_INTRA_BG = 0x6, // funct3 = 000
    AIM_AF_4BK_INTRA_BG = 0x7,  // funct3 = 001
    AIM_EWMUL = 0x8,            // funct3 = 010
    AIM_EWADD = 0x9,            // funct3 = 011
    AIM_MAC_ABK = 0xa,          // funct3 = 100
    AIM_AF_ABK = 0xb,           // funct3 = 101
    AIM_WR_AFLUT = 0xc,         // funct3 = 110
    AIM_WR_BK = 0xd,            // funct3 = 111
    // AIM_CUSTOM_0_MASK = FLOAD_MASK = 0x07
    AIM_WR_GB = 0xe,            // funct3 = 100
    AIM_WR_MAC = 0xf,           // funct3 = 101
    AIM_WR_BIAS = 0x10,         // funct3 = 110
    AIM_RD_MAC = 0x11,          // funct3 = 111
    // LOAD_MASK = 0x3
    AIM_RD_AF = 0x12,           // funct3 = 111
    // Deprecated
    // AIM_MAC_4BK_INTER_BG = 0x8, // funct3 = 010
    // AIM_AF_4BK_INTER_BG = 0x9,  // funct3 = 011
} MemAccessType;

typedef struct PendingMemAccessEntry
{
    int valid;
    int start_access;
    int access_size_bytes;
    target_ulong addr;
    target_ulong req_addr;
    target_ulong req_pte;
    int stage_queue_index;
    int stage_queue_type;
    MemAccessType type;
} PendingMemAccessEntry;

typedef struct StageMemAccessQueue
{
    int cur_idx;
    int max_size;
    int cur_size;
    PendingMemAccessEntry *entry;
} StageMemAccessQueue;
#endif