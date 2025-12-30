#include "pim_function.h"
#if TRACE_MODE
#include "AiM_trace.h"
#else
#include "AiM_asm.h"
#endif

volatile int *map_pim(void **out_map_base, size_t *out_map_size)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return NULL;
    }

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        perror("sysconf(_SC_PAGESIZE)");
        close(fd);
        return NULL;
    }

    off_t page_base   = PIM_BASE_ADDR & ~((off_t)page_size - 1);
    off_t page_offset = PIM_BASE_ADDR - page_base;
    size_t map_size   = (size_t)PIM_SIZE + (size_t)page_offset;

    void *map = mmap(NULL, map_size,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     fd, page_base);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }

    close(fd);

    if (out_map_base)
        *out_map_base = map;
    if (out_map_size)
        *out_map_size = map_size;

    volatile int *pim = (volatile int *)((uint8_t *)map + page_offset);
    return pim;
}

PIM_MANAGER* PIMregister(int n_ch)
{
    PIM_MANAGER* manager = (PIM_MANAGER*)malloc(sizeof(PIM_MANAGER));
    if (manager == NULL) {
        return NULL; 
    }
    manager->map_base = NULL;
    manager->map_size = 0;

    manager->pim_base = (uint16_t *)map_pim(&manager->map_base, &manager->map_size);
    if (!manager->pim_base) {
        return NULL;
    }

    manager->ch_begin = 0;
    manager->ch_end = n_ch;
    manager->next_idx = 0;
    
    return manager;
}

void PIMremove(PIM_MANAGER* manager)
{
    munmap(manager->map_base, manager->map_size);
    free(manager);
}

PIM_BUFFER* PIMmalloc(PIM_MANAGER* manager, int size, int mode)
{
    PIM_BUFFER *buffer = (PIM_BUFFER *)malloc(sizeof(PIM_BUFFER));
    if (!buffer) {
        perror("malloc PIM_BUFFER");
        return NULL;
    }

    int n_bank;
    int n_ch = manager->ch_end - manager->ch_begin;
    switch(mode){
        case 0: n_bank=n_ch*NUM_BGS*NUM_BANKS; break;
        case 1: n_bank=n_ch*NUM_BGS; break;
        case 2: n_bank=n_ch; break;
        default: return NULL;
    }
    int size_per_bank = size / n_bank;

    buffer->size = size;
    buffer->size_per_bank = size_per_bank;
    buffer->row_idx = manager->next_idx;
    buffer->mode = mode;

    int needed_rows = (size_per_bank / ROW_SIZE);
    if(needed_rows == 0){
        printf("[Error] PIMmalloc: dimension too small!\n");
        return NULL;
    }
    manager->next_idx += needed_rows;

    return buffer;
}

void PIMfree(PIM_BUFFER *pimbf)
{
    if(pimbf) free(pimbf);
}

int PIMmemcpy(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *hostData, int host_data_size){
    if(host_data_size != pimbf->size){
        printf("[Error] PIMmemcpy: dimension mismatch!\n");
        return 0;
    }

    // Mode 0: All Banks (CH -> BG -> BK)
    if(pimbf->mode == 0){
        for(int ch = manager->ch_begin; ch < manager->ch_end; ch++){
            for(int bg = 0; bg < NUM_BGS; bg++){
                for(int bk = 0; bk < NUM_BANKS; bk++){
                    int idx = (ch * NUM_BGS + bg) * NUM_BANKS + bk;
                    idx *= pimbf->size_per_bank;

                    for(int i = 0; i < (pimbf->size_per_bank / WORD_SIZE); i++){
                        int row = i / NUM_COLS + pimbf->row_idx;
                        int col = i % NUM_COLS;

                        uint64_t addr = addr_gen(ch, 0, bg, bk, row, col);

#if TRACE_MODE
                        printf("Addr ch: %d, bg: %d, bk: %d, row: %d, col: %d\n", ch, bg, bk, row, col);
#else
                        for(int offset = 0; offset < WORD_SIZE; offset++){
                            uint64_t byte_offset = addr + offset * sizeof(uint16_t);
                            volatile uint16_t *p = (volatile uint16_t *)((volatile uint8_t *)manager->pim_base + byte_offset);
                            *p = hostData[idx + i + offset];
                        }
#endif
                    }
                }
            }
        }
    }

    // Mode 1: Bank 0 of each BG (CH -> BG, BK fixed to 0)
    else if(pimbf->mode == 1){
        for(int ch = manager->ch_begin; ch < manager->ch_end; ch++){
            for(int bg = 0; bg < NUM_BGS; bg++){
                int bk = 0; 
                
                int idx = (ch * NUM_BGS + bg); 
                idx *= pimbf->size_per_bank;

                for(int i = 0; i < (pimbf->size_per_bank / WORD_SIZE); i++){
                    int row = i / NUM_COLS + pimbf->row_idx;
                    int col = i % NUM_COLS;

                    uint64_t addr = addr_gen(ch, 0, bg, bk, row, col);

#if TRACE_MODE
                    printf("Addr ch: %d, bg: %d, bk: %d, row: %d, col: %d\n", ch, bg, bk, row, col);
#else
                    for(int offset = 0; offset < WORD_SIZE; offset++){
                        uint64_t byte_offset = addr + offset * sizeof(uint16_t);
                        volatile uint16_t *p = (volatile uint16_t *)((volatile uint8_t *)manager->pim_base + byte_offset);
                        *p = hostData[idx + i + offset];
                    }
#endif
                }
            }
        }
    }

    // Mode 2: Bank 0 of BG 0 only (CH only, BG=0, BK=0)
    else if(pimbf->mode == 2){
        for(int ch = manager->ch_begin; ch < manager->ch_end; ch++){
            int idx = ch * pimbf->size_per_bank;

            for(int i = 0; i < (pimbf->size_per_bank / WORD_SIZE); i++){
                int row = i / NUM_COLS + pimbf->row_idx;
                int col = i % NUM_COLS;

                uint64_t addr = addr_gen(ch, 0, 0, 0, row, col);
#if TRACE_MODE
                printf("Addr ch: %d, bg: %d, bk: %d, row: %d, col: %d\n", ch, 0, 0, row, col);
#else
                for(int offset = 0; offset < WORD_SIZE; offset++){
                    uint64_t byte_offset = addr + offset * sizeof(uint16_t);
                    volatile uint16_t *p = (volatile uint16_t *)((volatile uint8_t *)manager->pim_base + byte_offset);
                    *p = hostData[idx + i + offset];
                }
#endif
            }
        }
    }

    else{
        return 0;
    }
    return 1;
}

static inline void load_input_to_pim(PIM_MANAGER *manager, int row_addr) {
    for (int col = 0; col < NUM_COLS; col++) {
        for (int ch = manager->ch_begin; ch < manager->ch_end; ch++) {
            aim_copy_bkgb((void *)manager->pim_base, addr_gen(ch, 0, 0, 0, row_addr, col));
        }
    }
}

static inline void execute_pim_mac(PIM_MANAGER *manager, int row_addr, int col_addr) {
    for (int ch = manager->ch_begin; ch < manager->ch_end; ch++) {
        aim_mac_abk((void *)manager->pim_base, addr_gen(ch, 0, 0, 0, row_addr, col_addr));
    }
}

static inline void read_pim_result(PIM_MANAGER *manager, uint16_t *output, int output_base_idx) {
    for (int ch = manager->ch_begin; ch < manager->ch_end; ch++) {
        uint16_t mac_result = aim_rd_mac((void *)manager->pim_base, addr_gen(ch, 0, 0, 0, 0, 0));
        output[output_base_idx + ch] = *(uint16_t *)&mac_result; 
    }
}

int PIMgemv(PIM_MANAGER *manager, PIM_BUFFER *input, PIM_BUFFER *weight, uint16_t *output, 
            int b, int in_h, int w_h, int w_r, int w_c)
{
    // 1. Dimension Checks
    if (input->size != b * in_h * w_r) {
        printf("[Error] PIMgemv: input dimension mismatch! (Expected: %d, Actual: %d)\n", b * in_h * w_r, input->size);
        return 0;
    }
    if (weight->size != b * w_h * w_r * w_c) {
        printf("[Error] PIMgemv: weight dimension mismatch! (Expected: %d, Actual: %d)\n", b * w_h * w_r * w_c, weight->size);
        return 0;
    }

    int h_ratio = (w_h > 0) ? (in_h / w_h) : 0;
    if (h_ratio == 0) {
        printf("[Error] PIMgemv: # heads of input is smaller than weight or div by zero!\n");
        return 0;
    }

    int n_ch = manager->ch_end - manager->ch_begin;
    int n_bank = n_ch * NUM_BGS * NUM_BANKS;
    int c_per_bank = w_h * w_c / n_bank;

    if (c_per_bank == 0) {
        printf("[Error] PIMgemv: weight dimension error (c_per_bank is 0)!\n");
        return 0;
    }

    // 2. Pre-calculation for Loops
    const int num_iters = weight->size_per_bank / (b * WORD_SIZE); 
    
    // Case A: Small Vector (w_r <= ROW_SIZE)
    if (w_r <= ROW_SIZE) {
        int vec_chunk_size = w_r / WORD_SIZE;
        int load_interval = vec_chunk_size * c_per_bank;

        int idx_in = 0;
        for (int idx_b = 0; idx_b < b; idx_b++) {            
            for (int idx_h = 0; idx_h < h_ratio; idx_h++) {
                for (int idx_w = 0; idx_w < num_iters; idx_w++) {
                    
                    // Step 1: Input Loading
                    if (idx_w % load_interval == 0) {
                        load_input_to_pim(manager, input->row_idx + idx_in);
                        idx_in++;
                    }

                    // Step 2: MAC Execution
                    int base_offset = idx_b * num_iters;
                    int weight_row_offset = (base_offset + idx_w) / NUM_COLS;
                    int weight_col = idx_w % NUM_COLS;

                    execute_pim_mac(manager, weight->row_idx + weight_row_offset, weight_col);

                    // Step 3: Read Result
                    if (idx_w % vec_chunk_size == (vec_chunk_size - 1)) {
                        int out_base_idx = (idx_w / w_r) * n_ch;
                        read_pim_result(manager, output, out_base_idx);
                    }
                }
            }
        }
    }
    // Case B: Large Vector (w_r > ROW_SIZE)
    else {
        int chunks_per_row = w_r / ROW_SIZE;
        int vec_chunk_limit = w_r / WORD_SIZE;

        for (int idx_b = 0; idx_b < b; idx_b++) {
            for (int idx_h = 0; idx_h < h_ratio; idx_h++) {
                int idx_in = 0;

                for (int idx_w = 0; idx_w < num_iters; idx_w++) {
                    
                    // Step 1: Input Loading
                    if (idx_w % NUM_COLS == 0) {
                        int row_offset = idx_in + (chunks_per_row * idx_h) + (chunks_per_row * h_ratio * idx_b);
                        load_input_to_pim(manager, input->row_idx + row_offset);
                        idx_in++;
                    }

                    // Step 2: MAC Execution
                    int base_offset = idx_b * num_iters;
                    int weight_row_offset = (base_offset + idx_w) / NUM_COLS;
                    int weight_col = idx_w % NUM_COLS;

                    execute_pim_mac(manager, weight->row_idx + weight_row_offset, weight_col);

                    // Step 3: Read Result
                    if (idx_w % vec_chunk_limit == (vec_chunk_limit - 1)) {
                        int out_base_idx = (idx_w / w_r) * n_ch;
                        read_pim_result(manager, output, out_base_idx);
                        
                        idx_in = 0; // Reset input tracking
                    }
                }
            }
        }
    }
    return 1;
}

int PIMupdate(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *new_vec, 
                int b, int h, int seq_idx, 
                int n_head, int s_len, int h_dim, int is_k) 
{
    int n_ch = manager->ch_end - manager->ch_begin;
    int n_bg = NUM_BGS;
    int n_bk = NUM_BANKS;
    int total_banks = n_ch * n_bg * n_bk;

    long total_elems_per_batch = (long)n_head * s_len * h_dim;
    long elems_per_bank = total_elems_per_batch / total_banks;

    if (elems_per_bank == 0) {
        printf("[Error] PIMupdateKV: Data size too small for bank partitioning.\n");
        return 0;
    }

    long batch_base_offset = (long)b * elems_per_bank;

    if (is_k) {
        long logical_start_idx = (long)h * (s_len * h_dim) + (long)seq_idx * h_dim;

        for (int d = 0; d < h_dim; d += WORD_SIZE) {
            long current_logical_idx = logical_start_idx + d;

            int target_bank_idx = current_logical_idx / elems_per_bank;
            long offset_in_bank = current_logical_idx % elems_per_bank;

            long total_offset = batch_base_offset + offset_in_bank;

            long word_chunk_idx = total_offset / WORD_SIZE;

            int row = word_chunk_idx / NUM_COLS + pimbf->row_idx;
            int col = word_chunk_idx % NUM_COLS;

            int bk = target_bank_idx % n_bk;
            int tmp = target_bank_idx / n_bk;
            int bg = tmp % n_bg;
            int ch = tmp / n_bg + manager->ch_begin;

            uint64_t addr = addr_gen(ch, 0, bg, bk, row, col);

#if TRACE_MODE
            printf("Addr ch: %d, bg: %d, bk: %d, row: %d, col: %d\n", ch, bg, bk, row, col);
#else
            for (int w = 0; w < WORD_SIZE; w++) {
                if (d + w >= h_dim) break;

                uint64_t byte_offset = addr + w * sizeof(uint16_t);
                volatile uint16_t *dst = (volatile uint16_t *)((volatile uint8_t *)manager->pim_base + byte_offset);
                *dst = new_vec[d + w];
            }
#endif
        }
    }
    // --- CASE 2: V Cache (Scatter Update) ---
    else {
        long head_base_idx = (long)h * (h_dim * s_len);

        for (int d = 0; d < h_dim; d++) {
            long current_logical_idx = head_base_idx + (long)d * s_len + seq_idx;

            int target_bank_idx = current_logical_idx / elems_per_bank;
            long offset_in_bank = current_logical_idx % elems_per_bank;
            long total_offset = batch_base_offset + offset_in_bank;

            long word_chunk_idx = total_offset / WORD_SIZE;
            int word_internal_offset = total_offset % WORD_SIZE; // 0 ~ 15

            int row = word_chunk_idx / NUM_COLS + pimbf->row_idx;
            int col = word_chunk_idx % NUM_COLS; // 0, 1, 2...

            int bk = target_bank_idx % n_bk;
            int tmp = target_bank_idx / n_bk;
            int bg = tmp % n_bg;
            int ch = tmp / n_bg + manager->ch_begin;

            uint64_t addr = addr_gen(ch, 0, bg, bk, row, col);
#if TRACE_MODE
            printf("Addr ch: %d, bg: %d, bk: %d, row: %d, col: %d\n", ch, bg, bk, row, col);
#else
            uint64_t byte_offset = addr + word_internal_offset * sizeof(uint16_t);
            volatile uint16_t *dst = (volatile uint16_t *)((volatile uint8_t *)manager->pim_base + byte_offset);
            
            *dst = new_vec[d];
#endif
        }
    }

    return 1;
}

int PIMupdateKV(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *new_vec_batch, 
                              int batch_size, int seq_idx, 
                              int n_head, int s_len, int h_dim, int is_k) 
{
    long size_per_batch = (long)n_head * h_dim;

    for (int b = 0; b < batch_size; b++) {
        for (int h = 0; h < n_head; h++) {
            long vec_offset = (long)b * size_per_batch + (long)h * h_dim;

            uint16_t *current_vec_ptr = new_vec_batch + vec_offset;
            int ret = PIMupdate(manager, pimbf, current_vec_ptr, b, h, seq_idx, n_head, s_len, h_dim, is_k);
            
            if (!ret) {
                printf("[Error] Failed to update KV at Batch:%d, Head:%d, Seq:%d\n", b, h, seq_idx);
                return 0;
            }
        }
    }
    return 1;
}

uint64_t addr_gen(unsigned int ch, unsigned int rank, unsigned int bg, unsigned int ba, unsigned int row, unsigned int col)
{
    uint64_t addr = 0;

    // Ch
    addr |= (uint64_t)ch;
    addr <<= NUM_RANK_BIT_;

    // Ra
    addr |= (uint64_t)rank;
    addr <<= NUM_BG_BIT_;

    // Bg
    addr |= (uint64_t)bg;
    addr <<= NUM_BANK_BIT_;

    // Bk
    addr |= (uint64_t)ba;
    addr <<= NUM_ROW_BIT_;

    // Ro
    addr |= (uint64_t)row;
    addr <<= NUM_COL_BIT_;

    // Ca
    addr |= (uint64_t)col;
    addr <<= NUM_OFFSET_BIT_;

    return addr;
}