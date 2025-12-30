#include "pim_function.h"
#include "function.h"
#include "def.h"

int main(void)
{
    void *map_base = NULL;
    size_t map_size = 0;

    volatile int *pim_base = map_pim(&map_base, &map_size);
    if (!pim_base) {
        return 0;
    }
    // int pim_base[100000] = {0, };

    uint16_t input[IN_DIM] ={0, };
    uint16_t weight[IN_DIM][OUT_DIM] ={0, };
    uint16_t output[OUT_DIM] ={0,};

    WeightShape input_shape = {1, 1, IN_DIM};
    WeightShape weight_shape = {1, IN_DIM, OUT_DIM};

    PIM_MANAGER manager;
    manager.pim_base = (uint16_t *)pim_base;
    manager.next_idx = 0;

    PIM_BUFFER *pim_input = PIMmallocSB(&manager, input_shape);
    PIM_BUFFER *pim_weight = PIMmalloc(&manager, weight_shape);

    if(PIM_SIM) SIM_START();
    PIMmemcpy(&manager, pim_input, (uint16_t *)input, pim_input->size);
    PIMmemcpy(&manager, pim_weight, (uint16_t *)weight, pim_weight->size);

    PIMgemv(&manager, pim_input, pim_weight, output);
    if(PIM_SIM) SIM_STOP();

    PIMfree(pim_input);
    PIMfree(pim_weight);

    munmap(map_base, map_size);
    return 0;
}
