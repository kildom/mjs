/*
 * Copyright (c) 2019 kildom
 * All rights reserved
 */

#ifndef md5_h_
#define md5_h_

#include <stdint.h>

struct md5
{
    uint32_t A;
    uint32_t B;
    uint32_t C;
    uint32_t D;
    uint64_t length;
    uint32_t chunk[16];
    uint32_t chunk_length;
};

void md5_init(struct md5* ctx);
void md5_update(struct md5* ctx, const unsigned char* data, int length);
void md5_finalize(struct md5* ctx, unsigned char* hash);
void md5_calc(const unsigned char* data, int length, unsigned char* hash);

#endif
