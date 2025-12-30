#ifndef PIM_MANAGER_H
#define PIM_MANAGER_H

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "def.h"

volatile int *map_pim(void **out_map_base, size_t *out_map_size);

typedef struct{
    uint32_t h;
    uint32_t r;
    uint32_t c;
} WeightShape;

typedef struct{
    WeightShape shape;
    int is_SB;
    int size;
    int size_per_bank;
    int row_idx;
} PIM_BUFFER;

typedef struct{
    volatile uint16_t* pim_base;
    int next_idx;
} PIM_MANAGER;

PIM_BUFFER* PIMmalloc(PIM_MANAGER* manager, WeightShape shape);
PIM_BUFFER* PIMmallocSB(PIM_MANAGER* manager, WeightShape shape);
void PIMfree(PIM_BUFFER *pimbf);
int PIMmemcpy(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *hostData, int host_data_size);

int PIMgemv(PIM_MANAGER *manager, PIM_BUFFER *input, PIM_BUFFER *weight, uint16_t *output);
int PIMupdateK(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *hostData, WeightShape shape);
int PIMupdateV(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *hostData, WeightShape shape);

uint64_t addr_gen(unsigned int ch, unsigned int rank, unsigned int bg, unsigned int ba, unsigned int row, unsigned int col);

#endif // PIM_MANAGER_H