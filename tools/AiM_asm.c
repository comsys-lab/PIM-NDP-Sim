#include <stdint.h>
#include "def.h"

/* AiM: RD_AF  (LOAD_MASK, funct3=7) */
uint16_t aim_rd_af(const void *base, int64_t offset)
{
    uint16_t value;
    const char *addr = (const char *)base + offset;

    /* I-type: .insn i opcode7, funct3, rd, simm12(rs1) */
    asm volatile (
        ".insn i %1, 7, %0, 0(%2)\n"
        : "=r"(value)
        : "i"(LOAD_MASK), "r"(addr)
        : "memory"
    );
    return value;
}

/* AiM S-type store instruction */
/* rs2: src, rs1: base, imm: offset */
#define AIM_S_TYPE_STORE(opcode, funct3, base_addr)             \
    asm volatile (                                                   \
        /* .insn s opcode7, funct3, rs2, simm12(rs1) */              \
        ".insn s %0, " #funct3 ", x0, 0(%1)\n"                       \
        :                                                            \
        : "i"(opcode), "r"(base_addr)                                \
        : "memory"                                                   \
    )

/* AIM_MAC_SBK  (STORE_MASK, funct3 = 4) */
void aim_mac_sbk(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(STORE_MASK, 4, addr);
}

/* AIM_AF_SBK   (STORE_MASK, funct3 = 5) */
void aim_af_sbk(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(STORE_MASK, 5, addr);
}

/* AIM_COPY_BKGB (STORE_MASK, funct3 = 6) */
void aim_copy_bkgb(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(STORE_MASK, 6, addr);
}

/* AIM_COPY_GBBK (STORE_MASK, funct3 = 7) */
void aim_copy_gbbk(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(STORE_MASK, 7, addr);
}

/* AiM: RD_MAC  (FLOAD_MASK, funct3 = 7) */
uint16_t aim_rd_mac(const void *base, int64_t offset)
{
    uint16_t value;
    const char *addr = (const char *)base + offset;

    asm volatile (
        ".insn i %1, 7, %0, 0(%2)\n"
        : "=r"(value)
        : "i"(FLOAD_MASK), "r"(addr)
        : "memory"
    );
    return value;
}

/* AIM_WR_GB (FLOAD_MASK, funct3 = 4) */
void aim_wr_gb(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(FLOAD_MASK, 4, addr);
}

/* AIM_WR_MAC (FLOAD_MASK, funct3 = 5) */
void aim_wr_mac(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(FLOAD_MASK, 5, addr);
}

/* AIM_WR_BIAS (FLOAD_MASK, funct3 = 6) */
void aim_wr_bias(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(FLOAD_MASK, 6, addr);
}

/* AIM_MAC_4BK_INTRA_BG (AIM_CUSTOM_1_MASK, funct3 = 0) */
void aim_mac_4bk_intra_bg(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(AIM_CUSTOM_1_MASK, 0, addr);
}

/* AIM_AF_4BK_INTRA_BG (AIM_CUSTOM_1_MASK, funct3 = 1) */
void aim_af_4bk_intra_bg(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(AIM_CUSTOM_1_MASK, 1, addr);
}

/* AIM_EWMUL (AIM_CUSTOM_1_MASK, funct3 = 2) */
void aim_ewmul(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(AIM_CUSTOM_1_MASK, 2, addr);
}

/* AIM_EWADD (AIM_CUSTOM_1_MASK, funct3 = 3) */
void aim_ewadd(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(AIM_CUSTOM_1_MASK, 3, addr);
}

/* AIM_MAC_ABK (AIM_CUSTOM_1_MASK, funct3 = 4) */
void aim_mac_abk(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(AIM_CUSTOM_1_MASK, 4, addr);
}

/* AIM_AF_ABK (AIM_CUSTOM_1_MASK, funct3 = 5) */
void aim_af_abk(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(AIM_CUSTOM_1_MASK, 5, addr);
}

/* AIM_WR_AFLUT (AIM_CUSTOM_1_MASK, funct3 = 6) */
void aim_wr_aflut(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(AIM_CUSTOM_1_MASK, 6, addr);
}

/* AIM_WR_BK (AIM_CUSTOM_1_MASK, funct3 = 7) */
void aim_wr_bk(volatile void *base, int64_t offset)
{
    char *addr = (char *)base + offset;
    AIM_S_TYPE_STORE(AIM_CUSTOM_1_MASK, 7, addr);
}