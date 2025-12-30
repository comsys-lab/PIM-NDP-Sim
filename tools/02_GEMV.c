#include "pim_function.h"
#include "function.h"

int main(void)
{
#if TRACE_MODE
    int pim_base[100] = {0, };
    PIM_MANAGER *manager = (PIM_MANAGER*)malloc(sizeof(PIM_MANAGER));
    manager->map_base = pim_base;
    manager->ch_begin = 0;
    manager->ch_end = 1;
    manager->next_idx = 0;
#else
    PIM_MANAGER *manager = PIMregister(1);
    if (!manager) {
        return 0;
    }
#endif

    int in_dim = 128;
    int out_dim = 1024;
    int b = 4;
    int in_h = 4;
    int w_h = 1;

    uint16_t output[1024];

    PIM_BUFFER *pim_input = PIMmalloc(manager, in_dim * b * in_h, 2);
    PIM_BUFFER *pim_weight = PIMmalloc(manager, in_dim * out_dim * b * w_h, 0);

#if PIM_SIM
    SIM_START();
#endif

    PIMgemv(manager, pim_input, pim_weight, output, b, in_h, w_h, in_dim, out_dim);

#if PIM_SIM
    SIM_STOP();
#endif

    PIMfree(pim_input);
    PIMfree(pim_weight);

#if TRACE_MODE
    free(manager);
#else
    PIMremove(manager);
#endif
    return 0;
}
