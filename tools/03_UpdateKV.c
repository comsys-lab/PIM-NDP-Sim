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
    uint16_t *K = (uint16_t *)malloc(hidden_dim/q_heads * sizeof(uint16_t));
    uint16_t *V = (uint16_t *)malloc(hidden_dim/q_heads * sizeof(uint16_t));

    PIM_BUFFER *pim_K = PIMmalloc(manager, hidden_dim/q_heads * seq_len * kv_heads * batch, 0);
    PIM_BUFFER *pim_V = PIMmalloc(manager, hidden_dim/q_heads * seq_len * kv_heads * batch, 0);
    
#if PIM_SIM
        SIM_START();
#endif

    int token_idx = 128;
    PIMupdateKV(manager, pim_K, K, batch, token_idx, kv_heads, seq_len, hidden_dim/q_heads, 1);
    PIMupdateKV(manager, pim_V, V, batch, token_idx, kv_heads, seq_len, hidden_dim/q_heads, 0);

#if PIM_SIM
        SIM_STOP();
#endif

    PIMfree(pim_K);
    PIMfree(pim_V);
#if TRACE_MODE
    free(manager);
#else
    PIMremove(manager);
#endif

    return 0;
}