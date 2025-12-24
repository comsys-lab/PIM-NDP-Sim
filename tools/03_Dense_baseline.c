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
    manager.pim_base = (_Float16 *)pim_base;
    manager.next_idx = 0;

    // Initialize Phase
    
    _Float16 K_cache[kv_heads] = {0, };
    _Float16 V_cache[kv_heads] = {0, };

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
    _Float16 *weight_Q = (_Float16*)malloc(hidden_dim * hidden_dim * sizeof(_Float16)); // Q Generation
    _Float16 *weight_K = (_Float16*)malloc(hidden_dim * hidden_dim / (q_heads / kv_heads) * sizeof(_Float16)); // K Generation
    _Float16 *weight_V = (_Float16*)malloc(hidden_dim * hidden_dim / (q_heads / kv_heads) * sizeof(_Float16)); // V Generation

    _Float16 *weight_O = (_Float16*)malloc(hidden_dim * hidden_dim * sizeof(_Float16)); // Output Projection

    _Float16 *weight_FC1 = (_Float16*)malloc(hidden_dim * intermediate_dim * sizeof(_Float16)); // Up Projection
    _Float16 *weight_FC2 = (_Float16*)malloc(hidden_dim * intermediate_dim * sizeof(_Float16)); // Down Projection

    // Intermediate Data
    _Float16 *X = (_Float16*)malloc(batch * hidden_dim * sizeof(_Float16)); // Input
    _Float16 *Y = (_Float16*)malloc(batch * hidden_dim * sizeof(_Float16)); // RMSNorm

    _Float16 *Q = (_Float16*)malloc(batch * hidden_dim * sizeof(_Float16)); // Q
    _Float16 *K = (_Float16*)malloc(batch * hidden_dim/ (q_heads / kv_heads) * sizeof(_Float16)); // K
    _Float16 *V = (_Float16*)malloc(batch * hidden_dim/ (q_heads / kv_heads) * sizeof(_Float16)); // V

    _Float16 *S = (_Float16*)malloc(batch * q_heads * seq_len * sizeof(_Float16)); // Scoring
    _Float16 *A = (_Float16*)malloc(batch * q_heads * seq_len * sizeof(_Float16)); // Softmax
    _Float16 *O = (_Float16*)malloc(batch * hidden_dim * sizeof(_Float16)); // output Projection

    _Float16 *Y2 = (_Float16*)malloc(batch * hidden_dim * sizeof(_Float16)); // LayerNorm
    _Float16 *C = (_Float16*)malloc(batch * intermediate_dim * sizeof(_Float16)); // Up Projection
    _Float16 *R = (_Float16*)malloc(batch * intermediate_dim * sizeof(_Float16)); // SwiGLU
    _Float16 *D = (_Float16*)malloc(batch * hidden_dim * sizeof(_Float16)); // Output Projection

    if (!weight_Q || !weight_K || !weight_V || !weight_O ||
        !weight_FC1 || !weight_FC2 || !X || !Y ||
        !Q || !K || !V || !S || !A || !O ||
        !Y2 || !C || !R || !D) {
        fprintf(stderr, "malloc failed\n");
        return -1;
    }

    // RMS Norm
    rmsnorm(X, Y, 1e-6f);
    // QKV Generation
    qkv_generation(Y, weight_Q, weight_K, weight_V, Q, K, V);

    // QK RMS Norm
    rmsnorm_qk(Q, K, 1e-5f);

    // RoPE
    apply_rope_qk(Q, K, 0);
    

    // Copy QKV to PIM
    PIMmemcpy(&manager, pim_Q, Q, pim_Q->size);

    K_shape.c = 128; // output token number
    V_shape.r = 128; // output token number
    PIMupdateK(&manager, pim_K, K, K_shape); // K cache update
    PIMupdateV(&manager, pim_V, V, V_shape); // V cache update

    // GQA Scoring
    if(!PIMgemv(&manager, pim_Q, pim_K, S)){
        printf("PIMgemv failed!\n");
        return -1;
    }

    // Softmax
    for(int head = 0; head < q_heads; head++){
        softmax(&S[head*seq_len], seq_len, &A[head*seq_len]);
    }
    PIMmemcpy(&manager, pim_A, A, pim_A->size);

    // Weighted Sum
    if(!PIMgemv(&manager, pim_A, pim_V, O)){
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