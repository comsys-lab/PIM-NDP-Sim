#include "pim_function2.h"


PIM_BUFFER* PIMmalloc(PIM_MANAGER* manager, WeightShape shape)
{
    PIM_BUFFER *buffer = (PIM_BUFFER *)malloc(sizeof(PIM_BUFFER));
    if (!buffer) {
        perror("malloc PIM_BUFFER");
        return NULL;
    }

    int host_data_size = shape.b * shape.h * shape.r * shape.c;
    int n_matrix_cols = shape.b * shape.h * shape.c;
    if (n_matrix_cols / (NUM_CHS * NUM_BGS * NUM_BANKS) == 0) {
        printf("[Warning] PIMmalloc: some banks are idle because weight matrix is too small\n");
    }

    int n_matrix_cols_per_bank = n_matrix_cols / (NUM_CHS * NUM_BGS * NUM_BANKS);

    if (host_data_size % (NUM_CHS * NUM_BGS * NUM_BANKS) != 0) {
        printf("[Error] PIMmalloc: host_data_size not aligned to bank/channel\n");
        free(buffer);
        return NULL;
    }

    int size_per_bank = host_data_size / (NUM_CHS * NUM_BGS * NUM_BANKS);
    int needed_rows = size_per_bank / ROW_SIZE + (size_per_bank % ROW_SIZE != 0);

    if (manager->next_idx + needed_rows > NUM_ROWS) {
        printf("[Error] PIMmalloc: out of PIM memory!\n");
        free(buffer);
        return NULL;
    }

    buffer->size = host_data_size;
    buffer->size_per_bank = size_per_bank;
    buffer->row_idx = manager->next_idx;
    buffer->shape = shape;
    buffer->is_SB = 0;
    buffer->n_matrix_cols_per_bank = n_matrix_cols_per_bank;
    buffer->n_vector_per_row = ROW_SIZE / shape.r;

    if (buffer->n_vector_per_row == 0) {
        printf("[Error] PIMmalloc: weight matrix row size larger than ROW_SIZE\n");
        free(buffer);
        return NULL;
    }

    manager->next_idx += needed_rows;

    return buffer;
}

// Assumption: Row dimension of weight matrices is smaller than DRAM row size. (1k elements)
int PIMgemv(PIM_MANAGER *manager, PIM_BUFFER *input, PIM_BUFFER *weight, uint16_t *output)
{
    if(weight->shape.r != input->shape.c){
        printf("[Error] PIMgemv: weight dimension mismatch!\n");
        return 0;
    }

    int h_ratio = input->shape.h / weight->shape.h;
    if(h_ratio == 0){
        printf("[Error] PIMgemv: input heads is smaller than weights!\n");
        return 0;
    }

    int n_MAC = weight->shape.r / WORD_SIZE;
    int n_rows_input = input->size_per_bank / (ROW_SIZE * h_ratio);
    int n_ins_row = (ROW_SIZE) / weight->shape.r;
    int n_row_per_bank = weight->shape.c / (NUM_BGS * NUM_BANKS * NUM_CHS);

    int cnt=0;

    for(int in_h_idx = 0; in_h_idx < input->shape.h; in_h_idx++)
    {
        for(int in_idx = 0; in_idx < n_MAC; in_idx++){
            for(int ch = 0; ch < NUM_CHS; ch++){
                int idx = in_h_idx * n_MAC + in_idx;
                aim_copy_bkgb(0, (void *)manager->pim_base, addr_gen(ch, 0, 0, 0, input->row_idx + idx / NUM_COLS , idx % NUM_COLS));
            }
        }

        for(int out_idx = 0; out_idx < n_row_per_bank; out_idx++){
            for(int mac_idx = 0; mac_idx < n_MAC; mac_idx++){
                int w_idx = (in_h_idx / h_ratio) * n_MAC * n_row_per_bank + n_MAC * out_idx + mac_idx;
                for(int ch = 0; ch < NUM_CHS; ch++){
                    aim_mac_abk(0, (void *)manager->pim_base, addr_gen(ch, 0, 0, 0, weight->row_idx + w_idx / NUM_COLS, w_idx % NUM_COLS));
                }
                cnt++;
            }

            for(int ch = 0; ch < NUM_CHS; ch++){
                uint16_t mac_result = aim_rd_mac((void *)manager->pim_base, addr_gen(ch, 0, 0, 0, 0, 0));
                output[ch*weight->shape.c*weight->shape.h + out_idx*(NUM_BGS * NUM_BANKS * NUM_CHS)] = *(uint16_t *)&mac_result; // Load single FP16 value
            }
        }
    }
    
    printf("PIM GEMV done...\n");

    return 1;
}

int PIMgemv(PIM_MANAGER *manager, PIM_BUFFER *input, PIM_BUFFER *weight, uint16_t *output, 
                            int b, int in_h, int w_h, int w_r, int w_c)
{
    // dimension check
    if(input->size != b * in_h * w_r){
        printf("[Error] PIMgemv: input dimension mismatch!\n");
        return 0;
    }

    if(weight->size != b * w_h * w_r * w_c){
        printf("[Error] PIMgemv: weight dimension mismatch!\n");
        return 0;
    }

    
}