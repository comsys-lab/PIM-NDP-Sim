#include "pim_function.h"
#include "def.h"

int main(void)
{
    void *map_base = NULL;
    size_t map_size = 0;

    volatile int *pim = map_pim(&map_base, &map_size);
    if (!pim) {
        return 1;
    }

    if(PIM_SIM) SIM_START();
    pim[0] = 0;
    if(PIM_SIM) SIM_STOP();

    munmap(map_base, map_size);
    return 0;
}
