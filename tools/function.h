#ifndef FUNCTION_H
#define FUNCTION_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
void rmsnorm(const uint16_t *, uint16_t *, float);
void qkv_generation(const uint16_t*, const uint16_t *, const uint16_t *, const uint16_t *, uint16_t *, uint16_t *, uint16_t *);
void rmsnorm_qk(uint16_t *, uint16_t *, float);
void softmax(const uint16_t *, int, uint16_t *);
void apply_rope_qk(uint16_t *, uint16_t *, int);
void output_projection(const uint16_t *, const uint16_t *, uint16_t *);
void residual_add_inplace(uint16_t *, const uint16_t *);
void up_projection(const uint16_t *, const uint16_t *, uint16_t *);
void swiglu(const uint16_t *, uint16_t *);
void down_projection(const uint16_t *, const uint16_t *, uint16_t *);

#endif // FUNCTION_H