#include "pim_function.h"

int main()
{
#if TRACE_MODE
    int pim_base[100] = {0, };
    PIM_MANAGER *manager = (PIM_MANAGER*)malloc(sizeof(PIM_MANAGER));
    manager->map_base = pim_base;
    manager->ch_begin = 0;
    manager->ch_end = 16;
    manager->next_idx = 0;
#else
    PIM_MANAGER *manager = PIMregister(16);
    if (!manager) {
        return 0;
    }
#endif
    uint16_t *data = (uint16_t *)malloc(1024 * 1024 * sizeof(uint16_t));
    PIM_BUFFER *pdata = PIMmalloc(manager, 1024 * 1024, 2);

#if PIM_SIM
    SIM_START();
#endif

    PIMmemcpy(manager, pdata, data, 1024*1024);

#if PIM_SIM
    SIM_STOP();
#endif

    PIMfree(pdata);
    free(data);

#if TRACE_MODE
    free(manager);
#else
    PIMremove(manager);
#endif

    return 0;
}