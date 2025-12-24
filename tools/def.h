#ifndef DEF_H
#define DEF_H

#define PIM_SIM 0

#define PIM_BASE_ADDR 0x480000000000
#define PIM_SIZE 0x1000000000 /* 64GB */

#define LOAD_MASK 0x3
#define STORE_MASK 0x23
#define FLOAD_MASK 0x07
#define FSTORE_MASK 0x27
#define AIM_CUSTOM_0_MASK 0x07
#define AIM_CUSTOM_1_MASK 0x2b

#define IN_DIM 1024
#define OUT_DIM 256

#define NUM_ROW_BIT_ 17
#define NUM_COL_BIT_ 5
#define NUM_BANK_BIT_ 2
#define NUM_BG_BIT_ 2
#define NUM_CH_BIT_ 4
#define NUM_RANK_BIT_ 0
#define NUM_OFFSET_BIT_ 5

#define WORD_SIZE 16 // data width: 32B = 16 FP16 numbers
#define NUM_ROWS (1 << NUM_ROW_BIT_)
#define NUM_COLS (1 << NUM_COL_BIT_)
#define NUM_BANKS (1 << NUM_BANK_BIT_)
#define NUM_BGS (1 << NUM_BG_BIT_)
#define NUM_CHS (1 << NUM_CH_BIT_)
#define BL (1 << NUM_OFFSET_BIT_) / WORD_SIZE
#define ROW_SIZE (WORD_SIZE * NUM_COLS)

#define hidden_dim 4096
#define q_heads 32
#define kv_heads 4
#define seq_len 1024
#define intermediate_dim 25600
#define batch 1

#define SIM_START() asm("csrs 0x800,zero")
#define SIM_STOP() asm("csrs 0x801,zero")

#endif /* DEF_H */