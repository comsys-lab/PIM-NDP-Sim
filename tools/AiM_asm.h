#ifndef AIM_ASM_H
#define AIM_ASM_H

#include <stdint.h>

uint16_t aim_rd_af(const void *base, int64_t offset);
void aim_mac_sbk(volatile void *base, int64_t offset);
void aim_af_sbk(volatile void *base, int64_t offset);
void aim_copy_bkgb(volatile void *base, int64_t offset);
void aim_copy_gbbk(volatile void *base, int64_t offset);
uint16_t aim_rd_mac(const void *base, int64_t offset);
void aim_wr_mac(volatile void *base, int64_t offset);
void aim_wr_gb(volatile void *base, int64_t offset);
void aim_wr_bias(volatile void *base, int64_t offset);
void aim_wr_bk(volatile void *base, int64_t offset);
void aim_mac_4bk_intra_bg(volatile void *base, int64_t offset);
void aim_af_4bk_intra_bg(volatile void *base, int64_t offset);
void aim_ewmul(volatile void *base, int64_t offset);
void aim_ewadd(volatile void *base, int64_t offset);

void aim_mac_abk(volatile void *base, int64_t offset);
void aim_af_abk(volatile void *base, int64_t offset);

void aim_wr_aflut(volatile void *base, int64_t offset);
void aim_wr_bk(volatile void *base, int64_t offset);

#endif // AIM_ASM_H