#include "function.h"
#include "def.h"

void rmsnorm(
    const uint16_t *X,   // [batch, hidden_dim]
    uint16_t *Y,         // [batch, hidden_dim]
    float epsilon
){
    for (int b = 0; b < batch; b++) {

        const uint16_t *x_ptr = X + b * hidden_dim;
        uint16_t       *y_ptr = Y + b * hidden_dim;

        float sum_sq = 0.0f;
        for (int i = 0; i < hidden_dim; i++) {
            float v = (float)x_ptr[i];
            sum_sq += v * v;
        }

        float mean_sq = sum_sq / (float)hidden_dim;
        float denom   = 1.0f / sqrtf(mean_sq + epsilon);

        for (int i = 0; i < hidden_dim; i++) {
            float v = (float)x_ptr[i];
            float r = v * denom;
            y_ptr[i] = (uint16_t)r;
        }
    }
}

void matmul_fp16_rowmajor(
    const uint16_t *A,  // [M, K]
    const uint16_t *B,  // [K, N]
    uint16_t *C,        // [M, N]
    int M,
    int K,
    int N
){
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n++) {
            float acc = 0.0f;
            const uint16_t *a_row = A + (size_t)m * K;
            const uint16_t *b_col = B + n; // B[k, n] = B[k*N + n]

            for (int k = 0; k < K; k++) {
                float av = (float)a_row[k];                  // A[m, k]
                float bv = (float)b_col[(size_t)k * N];      // B[k, n]
                acc += av * bv;
            }

            C[(size_t)m * N + n] = (uint16_t)acc;           // C[m, n]
        }
    }
}

void qkv_generation(
    const uint16_t *X,          // [batch, hidden_dim]
    const uint16_t *weight_Q,   // [hidden_dim, hidden_dim]
    const uint16_t *weight_K,   // [hidden_dim, k_dim]
    const uint16_t *weight_V,   // [hidden_dim, k_dim]
    uint16_t *Q,                // [batch, hidden_dim]
    uint16_t *K,                // [batch, k_dim]
    uint16_t *V                 // [batch, k_dim]
){
    // k_dim = hidden_dim / (q_heads / kv_heads)
    int factor = q_heads / kv_heads;   // 가정: q_heads % kv_heads == 0
    int k_dim  = hidden_dim / factor;

    // Q: [batch, hidden_dim] = [batch, hidden_dim] * [hidden_dim, hidden_dim]
    matmul_fp16_rowmajor(
        X,            // A: [batch, hidden_dim]
        weight_Q,     // B: [hidden_dim, hidden_dim]
        Q,            // C: [batch, hidden_dim]
        batch,
        hidden_dim,
        hidden_dim
    );

    // K: [batch, k_dim] = [batch, hidden_dim] * [hidden_dim, k_dim]
    matmul_fp16_rowmajor(
        X,            // A: [batch, hidden_dim]
        weight_K,     // B: [hidden_dim, k_dim]
        K,            // C: [batch, k_dim]
        batch,
        hidden_dim,
        k_dim
    );

    // V: [batch, k_dim] = [batch, hidden_dim] * [hidden_dim, k_dim]
    matmul_fp16_rowmajor(
        X,            // A: [batch, hidden_dim]
        weight_V,     // B: [hidden_dim, k_dim]
        V,            // C: [batch, k_dim]
        batch,
        hidden_dim,
        k_dim
    );
}

void output_projection(
    const uint16_t *attn_out,   // [batch, hidden_dim]
    const uint16_t *weight_O,   // [hidden_dim, hidden_dim]
    uint16_t *proj_out          // [batch, hidden_dim]
){
    // matmul: C[M, N] = A[M, K] * B[K, N]
    // A = attn_out           [batch, hidden_dim]
    // B = weight_O           [hidden_dim, hidden_dim]
    // C = proj_out           [batch, hidden_dim]

    matmul_fp16_rowmajor(
        attn_out,      // A
        weight_O,      // B
        proj_out,      // C
        batch,         // M
        hidden_dim,    // K
        hidden_dim     // N
    );
}

static inline void rmsnorm_single_vec_fp16(
    const uint16_t *x,
    uint16_t *y,
    int dim,
    float epsilon
){
    float sum_sq = 0.0f;
    for (int i = 0; i < dim; i++) {
        float v = (float)x[i];
        sum_sq += v * v;
    }

    float mean_sq = sum_sq / (float)dim;
    float inv_rms = 1.0f / sqrtf(mean_sq + epsilon);

    for (int i = 0; i < dim; i++) {
        float v = (float)x[i];
        y[i] = (uint16_t)(v * inv_rms);
    }
}

// Q: [batch, seq_len, q_heads, head_dim]
// K: [batch, seq_len, kv_heads, head_dim]
// in-place RMSNorm on Q and K
void rmsnorm_qk(
    uint16_t *Q,
    uint16_t *K,
    float epsilon
){
    const int head_dim = hidden_dim / q_heads;

    // --- Q: [batch, q_heads, head_dim] ---
    for (int b = 0; b < batch; b++) {
        for (int h = 0; h < q_heads; h++) {

            size_t offset =
                (size_t)b * hidden_dim +       // batch stride
                (size_t)h * head_dim;          // head stride

            uint16_t *q_vec = Q + offset;
            rmsnorm_single_vec_fp16(q_vec, q_vec, head_dim, epsilon);
        }
    }

    // --- K: [batch, kv_heads, head_dim] ---
    const int k_total_dim = hidden_dim / (q_heads / kv_heads); // = kv_heads * head_dim
    const int k_head_dim  = head_dim;                          // per-head dim은 Q와 동일

    for (int b = 0; b < batch; b++) {
        for (int h = 0; h < kv_heads; h++) {

            size_t offset =
                (size_t)b * k_total_dim +       // batch stride
                (size_t)h * k_head_dim;         // head stride

            uint16_t *k_vec = K + offset;
            rmsnorm_single_vec_fp16(k_vec, k_vec, k_head_dim, epsilon);
        }
    }
}

void softmax(const uint16_t *scores, int len, uint16_t *probs)
{
    const int L = len;
    float tmp[L];

    for (int b = 0; b < batch; b++) {
        for (int h = 0; h < q_heads; h++) {
            size_t base = ((size_t)b * q_heads + (size_t)h) * (size_t)L;

            float max_val = -INFINITY;
            for (int t = 0; t < L; t++) {
                float s = (float)scores[base + t];
                if (s > max_val) max_val = s;
            }

            float sum = 0.0f;
            for (int t = 0; t < L; t++) {
                float s = (float)scores[base + t];
                float e = expf(s - max_val);
                tmp[t] = e;
                sum += e;
            }

            float inv_sum = 1.0f / sum;
            for (int t = 0; t < L; t++) {
                float p = tmp[t] * inv_sum;
                probs[base + t] = (uint16_t)p;
            }
        }
    }
}

static inline float rope_theta(int pos, int pair_idx, int head_dim)
{
    // pair_idx: 0,1,2,... for (2i, 2i+1)
    // d = head_dim
    float inv_freq = powf(10000.0f, -(2.0f * (float)pair_idx) / (float)head_dim);
    return (float)pos * inv_freq;
}

// Q: [batch, seq_len, q_heads, head_dim]
// K: [batch, seq_len, kv_heads, head_dim]
// in-place RoPE
void apply_rope_qk(
    uint16_t *Q,   // [batch, q_heads, head_dim]  (flatten: batch * hidden_dim)
    uint16_t *K,   // [batch, kv_heads, head_dim] (flatten: batch * k_total_dim)
    int pos        // 현재 토큰 position
){
    const int head_dim   = hidden_dim / q_heads;
    const int k_total_dim = hidden_dim / (q_heads / kv_heads);
    const int k_head_dim  = head_dim;

    // Q
    for (int b = 0; b < batch; b++) {
        for (int h = 0; h < q_heads; h++) {
            size_t offset =
                (size_t)b * hidden_dim +
                (size_t)h * head_dim;

            uint16_t *q_vec = Q + offset;

            for (int d = 0; d < head_dim; d += 2) {
                int pair_idx = d / 2;
                float theta = rope_theta(pos, pair_idx, head_dim);
                float c = cosf(theta);
                float s = sinf(theta);

                float q0 = (float)q_vec[d];
                float q1 = (float)q_vec[d + 1];

                float q0_new = q0 * c - q1 * s;
                float q1_new = q0 * s + q1 * c;

                q_vec[d]     = (uint16_t)q0_new;
                q_vec[d + 1] = (uint16_t)q1_new;
            }
        }
    }

    // K
    for (int b = 0; b < batch; b++) {
        for (int h = 0; h < kv_heads; h++) {
            size_t offset =
                (size_t)b * k_total_dim +
                (size_t)h * k_head_dim;

            uint16_t *k_vec = K + offset;

            for (int d = 0; d < k_head_dim; d += 2) {
                int pair_idx = d / 2;
                float theta = rope_theta(pos, pair_idx, k_head_dim);
                float c = cosf(theta);
                float s = sinf(theta);

                float k0 = (float)k_vec[d];
                float k1 = (float)k_vec[d + 1];

                float k0_new = k0 * c - k1 * s;
                float k1_new = k0 * s + k1 * c;

                k_vec[d]     = (uint16_t)k0_new;
                k_vec[d + 1] = (uint16_t)k1_new;
            }
        }
    }
}

void residual_add_inplace(
    uint16_t *X_inout,
    const uint16_t *proj_out
){
    const int total = batch * hidden_dim;
    for (int i = 0; i < total; i++) {
        float x = (float)X_inout[i];
        float y = (float)proj_out[i];
        X_inout[i] = (uint16_t)(x + y);
    }
}

void up_projection(
    const uint16_t *Y2,
    const uint16_t *weight_FC1,
    uint16_t *C
){
    matmul_fp16_rowmajor(
        Y2,          // A: [batch, hidden_dim]
        weight_FC1,  // B: [hidden_dim, intermediate_dim]
        C,           // C: [batch, intermediate_dim]
        batch,       // M
        hidden_dim,  // K
        intermediate_dim // N
    );
}

void swiglu(
    const uint16_t *C,
    uint16_t *R
){
    const int dim = intermediate_dim;  // per-sample output dim

    for (int b = 0; b < batch; b++) {

        const uint16_t *c_ptr = C + (size_t)b * (2 * dim);
        uint16_t       *r_ptr = R + (size_t)b * dim;

        // U = c_ptr[0 .. dim-1]
        // V = c_ptr[dim .. 2*dim-1]
        for (int i = 0; i < dim; i++) {
            float u = (float)c_ptr[i];
            float v = (float)c_ptr[i + dim];

            // Swish(u) = u * sigmoid(u)
            float sig = 1.0f / (1.0f + expf(-u));
            float swish_u = u * sig;

            float out = swish_u * v;
            r_ptr[i] = (uint16_t)out;
        }
    }
}

void down_projection(
    const uint16_t *R,
    const uint16_t *weight_FC2,
    uint16_t *D
){
    matmul_fp16_rowmajor(
        R,           // A: [batch, intermediate_dim]
        weight_FC2,  // B: [intermediate_dim, hidden_dim]
        D,           // C: [batch, hidden_dim]
        batch,       // M
        intermediate_dim, // K
        hidden_dim   // N
    );
}