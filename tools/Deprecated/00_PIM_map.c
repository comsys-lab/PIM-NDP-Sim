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
    for (int i = 0 ; i < 1024 ; i++){
        pim[i * 32] = i;
    }
    if(PIM_SIM) SIM_STOP();

    munmap(map_base, map_size);
    return 0;
}
