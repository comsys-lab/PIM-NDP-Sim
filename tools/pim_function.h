#ifndef PIM_MANAGER_H
#define PIM_MANAGER_H

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "def.h"

typedef struct{
    int size;
    int size_per_bank;
    int mode; // 0: abk, 1: 4bk, 2: sbk
    int row_idx;
} PIM_BUFFER;

typedef struct{
    volatile uint16_t* pim_base;
    int ch_begin;
    int ch_end;
    int next_idx;

    void *map_base;
    size_t map_size;
} PIM_MANAGER;

volatile int* map_pim(void **out_map_base, size_t *out_map_size);
PIM_MANAGER* PIMregister(int n_ch);
void PIMremove(PIM_MANAGER* manager);

PIM_BUFFER* PIMmalloc(PIM_MANAGER* manager, int size, int mode);
void PIMfree(PIM_BUFFER *pimbf);
int PIMmemcpy(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *hostData, int host_data_size);

int PIMgemv(PIM_MANAGER *manager, PIM_BUFFER *input, PIM_BUFFER *weight, uint16_t *output, int b, int in_h, int w_h, int w_r, int w_c);
int PIMupdateKV(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *new_vec_batch, 
                              int batch_size, int seq_idx, 
                              int n_head, int s_len, int h_dim, int is_k);
uint64_t addr_gen(unsigned int ch, unsigned int rank, unsigned int bg, unsigned int ba, unsigned int row, unsigned int col);

#endif // PIM_MANAGER_H