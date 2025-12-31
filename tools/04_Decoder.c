#include "pim_function.h"
#include "function.h"

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

    // Initialize Phase

    PIM_BUFFER *pim_Q = PIMmalloc(manager, batch * hidden_dim, 2);
    PIM_BUFFER *pim_K = PIMmalloc(manager, kv_heads * hidden_dim / q_heads * seq_len, 0);
    PIM_BUFFER *pim_A = PIMmalloc(manager, q_heads * seq_len, 2);
    PIM_BUFFER *pim_V = PIMmalloc(manager, kv_heads * seq_len * hidden_dim / q_heads, 0);

    // Weight Matrices for Host
    // Assumption: KV caches already stored in PIM region.
    uint16_t *weight_Q = (uint16_t*)malloc(hidden_dim * hidden_dim * sizeof(uint16_t)); // Q Generation
    uint16_t *weight_K = (uint16_t*)malloc(hidden_dim * hidden_dim / (q_heads / kv_heads) * sizeof(uint16_t)); // K Generation
    uint16_t *weight_V = (uint16_t*)malloc(hidden_dim * hidden_dim / (q_heads / kv_heads) * sizeof(uint16_t)); // V Generation

    uint16_t *weight_O = (uint16_t*)malloc(hidden_dim * hidden_dim * sizeof(uint16_t)); // Output Projection

    uint16_t *weight_FC1 = (uint16_t*)malloc(hidden_dim * intermediate_dim * sizeof(uint16_t)); // Up Projection
    uint16_t *weight_FC2 = (uint16_t*)malloc(hidden_dim * intermediate_dim * sizeof(uint16_t)); // Down Projection

    // Intermediate Data
    uint16_t *X = (uint16_t*)malloc(batch * hidden_dim * sizeof(uint16_t)); // Input
    uint16_t *Y = (uint16_t*)malloc(batch * hidden_dim * sizeof(uint16_t)); // RMSNorm

    uint16_t *Q = (uint16_t*)malloc(batch * hidden_dim * sizeof(uint16_t)); // Q
    uint16_t *K = (uint16_t*)malloc(batch * hidden_dim/ (q_heads / kv_heads) * sizeof(uint16_t)); // K
    uint16_t *V = (uint16_t*)malloc(batch * hidden_dim/ (q_heads / kv_heads) * sizeof(uint16_t)); // V

    uint16_t *S = (uint16_t*)malloc(batch * q_heads * seq_len * sizeof(uint16_t)); // Scoring
    uint16_t *A = (uint16_t*)malloc(batch * q_heads * seq_len * sizeof(uint16_t)); // Softmax
    uint16_t *O = (uint16_t*)malloc(batch * hidden_dim * sizeof(uint16_t)); // output Projection

    uint16_t *Y2 = (uint16_t*)malloc(batch * hidden_dim * sizeof(uint16_t)); // LayerNorm
    uint16_t *C = (uint16_t*)malloc(batch * intermediate_dim * sizeof(uint16_t)); // Up Projection
    uint16_t *R = (uint16_t*)malloc(batch * intermediate_dim * sizeof(uint16_t)); // SwiGLU
    uint16_t *D = (uint16_t*)malloc(batch * hidden_dim * sizeof(uint16_t)); // Output Projection

    if (!weight_Q || !weight_K || !weight_V || !weight_O ||
        !weight_FC1 || !weight_FC2 || !X || !Y ||
        !Q || !K || !V || !S || !A || !O ||
        !Y2 || !C || !R || !D) {
        fprintf(stderr, "malloc failed\n");
        return -1;
    }

#if PIM_SIM
    SIM_START();
#endif

    // RMS Norm
    rmsnorm(X, Y, 1e-6f);
    // QKV Generation
    qkv_generation(Y, weight_Q, weight_K, weight_V, Q, K, V);
    // QK RMS Norm
    rmsnorm_qk(Q, K, 1e-5f);
    // RoPE
    apply_rope_qk(Q, K, 0);
    // Copy QKV to PIM

    PIMmemcpy(manager, pim_Q, Q, pim_Q->size);

    int token_idx = 128;
    PIMupdateKV(manager, pim_K, K, batch, token_idx, kv_heads, seq_len, hidden_dim/q_heads, 1);
    PIMupdateKV(manager, pim_V, V, batch, token_idx, kv_heads, seq_len, hidden_dim/q_heads, 0);

    // GQA Scoring
    if(!PIMgemv(manager, pim_Q, pim_K, S, batch, q_heads, kv_heads, hidden_dim/q_heads, seq_len)){
        printf("PIMgemv failed!\n");
        return -1;
    }

    // Softmax
    softmax(S, seq_len, A);

    // Weighted Sum
    PIMmemcpy(manager, pim_A, A, pim_A->size);
    if(!PIMgemv(manager, pim_A, pim_V, O, batch, q_heads, kv_heads, seq_len, hidden_dim/q_heads)){
        printf("PIMgemv failed!\n");
        return -1;
    }

    // Output Projection
    output_projection(O, weight_O, O);

    // Residual
    residual_add_inplace(X, O);

    // RMS Norm
    rmsnorm(X, Y2, 1e-5f);
    
    // Up Projection
    up_projection(Y2, weight_FC1, C);

    // SwiGLU
    swiglu(C, R);

    // Down Projection
    down_projection(R, weight_FC2, D);

    // Residual
    residual_add_inplace(X, D);

#if PIM_SIM
    SIM_STOP();
#endif

    // Cleanup Phase
    free(weight_Q);
    free(weight_K);
    free(weight_V);
    free(weight_O);
    free(weight_FC1);
    free(weight_FC2);

    free(X);
    free(Y);
    free(Q);
    free(K);
    free(V);
    free(S);
    free(A);
    free(O);
    free(Y2);
    free(C);
    free(R);
    free(D);

    PIMfree(pim_Q);
    PIMfree(pim_K);
    PIMfree(pim_A);
    PIMfree(pim_V);

#if TRACE_MODE
    free(manager);
#else
    PIMremove(manager);
#endif
    return 0;
}