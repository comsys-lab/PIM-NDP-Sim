#include "pim_function.h"
#include "function.h"
#include "def.h"

int main()
{
    void *map_base = NULL;
    size_t map_size = 0;

    // volatile int *pim_base = map_pim(&map_base, &map_size);
    // if (!pim_base) {
    //     return 0;
    // }
    int pim_base[10] = {0, };
    
    PIM_MANAGER manager;
    manager.pim_base = (uint16_t *)pim_base;
    manager.next_idx = 0;

    // Initialize Phase
    WeightShape Q_shape = {q_heads, 1, hidden_dim / q_heads};
    WeightShape K_shape = {kv_heads, hidden_dim / q_heads, seq_len};
    WeightShape A_shape = {q_heads, 1, seq_len};
    WeightShape V_shape = {kv_heads, seq_len, hidden_dim / q_heads};

    PIM_BUFFER *pim_Q = PIMmallocSB(&manager, Q_shape);
    PIM_BUFFER *pim_K = PIMmalloc(&manager, K_shape);
    PIM_BUFFER *pim_A = PIMmallocSB(&manager, A_shape);
    PIM_BUFFER *pim_V = PIMmalloc(&manager, V_shape);

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

    if(PIM_SIM) SIM_START();

    // RMS Norm
    rmsnorm(X, Y, 1e-6f);
    // QKV Generation
    qkv_generation(Y, weight_Q, weight_K, weight_V, Q, K, V);
    // QK RMS Norm
    rmsnorm_qk(Q, K, 1e-5f);
    // RoPE
    apply_rope_qk(Q, K, 0);
    // Copy QKV to PIM
    for(int b = 0; b < batch; b++){
        PIMmemcpy(&manager, pim_Q, Q, pim_Q->size);

        K_shape.c = 128; // output token number
        V_shape.r = 128; // output token number
        PIMupdateK(&manager, pim_K, K, K_shape); // K cache update
        PIMupdateV(&manager, pim_V, V, V_shape); // V cache update
    }

    // GQA Scoring
    for(int b = 0; b < batch; b++)
        if(!PIMgemv(&manager, pim_Q, pim_K, S)){
            printf("PIMgemv failed!\n");
            return -1;
        }

    // Softmax
    softmax(S, seq_len, A);

    // Weighted Sum
    for(int b = 0; b < batch; b++){
        PIMmemcpy(&manager, pim_A, A, pim_A->size);
        if(!PIMgemv(&manager, pim_A, pim_V, O)){
            printf("PIMgemv failed!\n");
            return -1;
        }
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

    if(PIM_SIM) SIM_STOP();

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

    munmap(map_base, map_size);
    return 0;
}