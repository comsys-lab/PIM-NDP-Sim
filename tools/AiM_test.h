#ifndef AIM_TEST_H
#define AIM_TEST_H

#include <stdio.h>
#include <stdint.h>
#include "def.h"

// Debug AiM assembly functions

/* AiM: RD_AF  (LOAD_MASK, funct3=7) */
uint16_t aim_rd_af(const void *base, uint64_t offset)
{
    uint16_t value;
    const char *addr = (const char *)base + offset;

    printf("aim_rd_af %d\n", offset);
    return value;
}

/* AIM_MAC_SBK  (STORE_MASK, funct3 = 4) */
void aim_mac_sbk(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    
    printf("aim_mac_sbk %p\n", addr);
}

/* AIM_AF_SBK   (STORE_MASK, funct3 = 5) */
void aim_af_sbk(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_af_sbk %p\n", addr);
}

/* AIM_COPY_BKGB (STORE_MASK, funct3 = 6) */
void aim_copy_bkgb(void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_copy_bkgb %lx\n", offset);
}

/* AIM_COPY_GBBK (STORE_MASK, funct3 = 7) */
void aim_copy_gbbk(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_copy_gbbk %p\n", addr);
}

/* AiM: RD_MAC  (FLOAD_MASK, funct3 = 7) */
uint16_t aim_rd_mac(const void *base, uint64_t offset)
{
    uint16_t value;
    const char *addr = (const char *)base + offset;

    printf("aim_rd_mac %p\n", addr);
    return value;
}

/* AIM_WR_GB (FLOAD_MASK, funct3 = 4) */
void aim_wr_gb(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_wr_gb %p\n", addr);
}

/* AIM_WR_MAC (FLOAD_MASK, funct3 = 5) */
void aim_wr_mac(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_wr_mac %p\n", addr);
}

/* AIM_WR_BIAS (FLOAD_MASK, funct3 = 6) */
void aim_wr_bias(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_wr_bias %p\n", addr);
}

/* AIM_MAC_4BK_INTRA_BG (AIM_CUSTOM_1_MASK, funct3 = 0) */
void aim_mac_4bk_intra_bg(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_mac_4bk_intra_bg %p\n", addr);
}

/* AIM_AF_4BK_INTRA_BG (AIM_CUSTOM_1_MASK, funct3 = 1) */
void aim_af_4bk_intra_bg(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_af_4bk_intra_bg %p\n", addr);
}

/* AIM_EWMUL (AIM_CUSTOM_1_MASK, funct3 = 2) */
void aim_ewmul(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_ewmul %p\n", addr);
}

/* AIM_EWADD (AIM_CUSTOM_1_MASK, funct3 = 3) */
void aim_ewadd(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_ewadd %p\n", addr);
}

/* AIM_MAC_ABK (AIM_CUSTOM_1_MASK, funct3 = 4) */
void aim_mac_abk(void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_mac_abk %lx\n", offset);
}

/* AIM_AF_ABK (AIM_CUSTOM_1_MASK, funct3 = 5) */
void aim_af_abk(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_af_abk %p\n", addr);
}

/* AIM_WR_AFLUT (AIM_CUSTOM_1_MASK, funct3 = 6) */
void aim_wr_aflut(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_wr_aflut %p\n", addr);
}

/* AIM_WR_BK (AIM_CUSTOM_1_MASK, funct3 = 7) */
void aim_wr_bk(uint64_t src, void *base, uint64_t offset)
{
    char *addr = (char *)base + offset;
    printf("aim_wr_bk %p\n", addr);
}

#endif // AIM_TEST_H