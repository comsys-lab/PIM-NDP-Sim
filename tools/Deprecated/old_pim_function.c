#include "pim_function.h"
#include "AiM_test.h"
// #include "AiM_asm.h"

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

PIM_BUFFER* PIMmalloc(PIM_MANAGER* manager, WeightShape shape)
{
    PIM_BUFFER *buffer = (PIM_BUFFER *)malloc(sizeof(PIM_BUFFER));
    if (!buffer) {
        perror("malloc PIM_BUFFER");
        return NULL;
    }

    int host_data_size = shape.h * shape.r * shape.c;
    if (host_data_size % (NUM_CHS * NUM_BGS * NUM_BANKS) != 0) {
        printf("[Error] PIMmalloc: host_data_size not aligned to bank/channel\n");
        return NULL;
    }

    int size_per_bank = host_data_size / (NUM_CHS * NUM_BGS * NUM_BANKS);
    int needed_rows = size_per_bank / ROW_SIZE + (size_per_bank % ROW_SIZE != 0);

    if (manager->next_idx + needed_rows > NUM_ROWS) {
        printf("[Error] PIMmalloc: out of PIM memory!\n");
        return NULL;
    }

    buffer->size = host_data_size;
    buffer->size_per_bank = size_per_bank;
    buffer->row_idx = manager->next_idx;
    buffer->shape = shape;
    buffer->is_SB = 0;

    manager->next_idx += needed_rows;

    return buffer;
}

PIM_BUFFER* PIMmallocSB(PIM_MANAGER* manager, WeightShape shape)
{
    PIM_BUFFER *buffer = (PIM_BUFFER *)malloc(sizeof(PIM_BUFFER));
    if (!buffer) {
        perror("malloc PIM_BUFFER");
        return NULL;
    }

    int size_per_bank = shape.h * shape.r * shape.c;
    int needed_rows = size_per_bank / ROW_SIZE + (size_per_bank % ROW_SIZE != 0);

    if (manager->next_idx + needed_rows > NUM_ROWS) {
        printf("[Error] PIMmalloc: out of PIM memory!\n");
        return NULL;
    }

    buffer->size = size_per_bank;
    buffer->size_per_bank = size_per_bank;
    buffer->row_idx = manager->next_idx;
    buffer->shape = shape;
    buffer->is_SB = 1;

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

    if(!pimbf->is_SB){
        for(int ch = 0; ch < NUM_CHS; ch++){
            for(int bg = 0; bg < NUM_BGS; bg++){
                for(int bk = 0; bk < NUM_BANKS; bk++){
                    int idx = (ch * NUM_BGS + bg) * NUM_BANKS + bk;
                    idx *= pimbf->size_per_bank;

                    for(int i = 0; i < pimbf->size_per_bank; i += WORD_SIZE){
                        int row = i / ROW_SIZE + pimbf->row_idx;
                        int col = i % ROW_SIZE;

                        uint64_t addr = addr_gen(ch, 0, bg, bk, row, col);
                        for(int offset = 0; offset < WORD_SIZE; offset++){
                            uint64_t byte_offset = addr + offset * sizeof(uint16_t);
                            volatile uint16_t *p = (volatile uint16_t *)((volatile uint8_t *)manager->pim_base + byte_offset);
                            //*p = hostData[idx + i + offset];
                        }
                    }
                }
            }
        }
    }

    else{
        for(int ch = 0; ch < NUM_CHS; ch++){
            int idx = ch * pimbf->size_per_bank;

            for(int i = 0; i < pimbf->size_per_bank; i += WORD_SIZE){
                int row = i / ROW_SIZE + pimbf->row_idx;
                int col = i % ROW_SIZE;

                uint64_t addr = addr_gen(ch, 0, 0, 0, row, col);
                for(int offset = 0; offset < WORD_SIZE; offset++){
                    uint64_t byte_offset = addr + offset * sizeof(uint16_t);
                    volatile uint16_t *p = (volatile uint16_t *)((volatile uint8_t *)manager->pim_base + byte_offset);
                    // printf("Writing to addr: %p value: %f\n", p, hostData[idx + i + offset]);
                    //*p = hostData[idx + i + offset];
                }
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
                aim_copy_bkgb((void *)manager->pim_base, addr_gen(ch, 0, 0, 0, input->row_idx + idx / NUM_COLS , idx % NUM_COLS));
            }
        }

        for(int out_idx = 0; out_idx < n_row_per_bank; out_idx++){
            for(int mac_idx = 0; mac_idx < n_MAC; mac_idx++){
                int w_idx = (in_h_idx / h_ratio) * n_MAC * n_row_per_bank + n_MAC * out_idx + mac_idx;
                for(int ch = 0; ch < NUM_CHS; ch++){
                    aim_mac_abk((void *)manager->pim_base, addr_gen(ch, 0, 0, 0, weight->row_idx + w_idx / NUM_COLS, w_idx % NUM_COLS));
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

int PIMupdateK(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *hostData, WeightShape shape)
{
    if(pimbf->shape.h != shape.h || pimbf->shape.r != shape.r){
        printf("[Error] PIMupdate: shape mismatch!\n");
        return 0;
    }

    int n_cols = pimbf->shape.c * pimbf->shape.h;
    int n_cols_per_ch = n_cols / NUM_CHS;
    int n_cols_per_bg = n_cols / (NUM_BGS * NUM_CHS);
    int n_cols_per_bank = n_cols / (NUM_BGS * NUM_BANKS * NUM_CHS);
    n_cols /= shape.h;

    for(int h = 0; h < shape.h; h++){
        int c = shape.c + h*n_cols;
        int ch = c / n_cols_per_ch;
        int bg = c % n_cols_per_ch / n_cols_per_bg;
        int bk = c % n_cols_per_bg / n_cols_per_bank;

        int idx = (c % n_cols_per_bank) * shape.r / WORD_SIZE;
        int row = idx / NUM_COLS + pimbf->row_idx;
        int col = idx % NUM_COLS;

        for(int r = 0; r < shape.r; r += WORD_SIZE){
            uint64_t addr = addr_gen(ch, 0, bg, bk, row, col);
            for(int offset = 0; offset < WORD_SIZE; offset++){
                uint64_t byte_offset = addr + offset * sizeof(uint16_t);
                volatile uint16_t *p = (volatile uint16_t *)((volatile uint8_t *)manager->pim_base + byte_offset);
                // printf("Writing to addr: %p value: %f\n", p, hostData[h * shape.r + r + offset]);
                //*p = hostData[h * shape.r + r + offset];
            }
            col++;
            if(col == NUM_COLS){
                col = 0;
                row++;
            }
        }
    }
    
    return 1;
}

int PIMupdateV(PIM_MANAGER *manager, PIM_BUFFER *pimbf, uint16_t *hostData, WeightShape shape)
{
    if(pimbf->shape.h != shape.h || pimbf->shape.c != shape.c){
        printf("[Error] PIMupdate: shape mismatch!\n");
        return 0;
    }

    int n_rows = pimbf->shape.c * pimbf->shape.h;
    int n_rows_per_ch = n_rows / NUM_CHS;
    int n_rows_per_bg = n_rows / (NUM_BGS * NUM_CHS);
    int n_rows_per_bank = n_rows / (NUM_BGS * NUM_BANKS * NUM_CHS);

    for(int h = 0; h < pimbf->shape.h; h++){
        for(int r = 0; r < pimbf->shape.c; r++){
            int idx = h * pimbf->shape.r * pimbf->shape.c + r * pimbf->shape.r + shape.r;

            int ch = idx / (NUM_BGS * NUM_BANKS * pimbf->size_per_bank);
            int bg = (idx % (NUM_BGS * NUM_BANKS * pimbf->size_per_bank)) / (NUM_BANKS * pimbf->size_per_bank);
            int bk = (idx % (NUM_BANKS * pimbf->size_per_bank)) / pimbf->size_per_bank;
            int row = (idx % pimbf->size_per_bank) / ROW_SIZE + pimbf->row_idx;
            int col = (idx % pimbf->size_per_bank) % ROW_SIZE / WORD_SIZE;
            int offset = (idx % pimbf->size_per_bank) % ROW_SIZE % WORD_SIZE;

            uint64_t addr = addr_gen(ch, 0, bg, bk, row, col);
            uint64_t byte_offset = addr + offset * sizeof(uint16_t);
            volatile uint16_t *p = (volatile uint16_t *)((volatile uint8_t *)manager->pim_base + byte_offset);
            // printf("Writing to addr: %p value: %f\n", p, hostData[h * shape.r + r]);
            //*p = hostData[h * shape.r + r];
        }
    }
    return 1;
}
