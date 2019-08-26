/*
 * Copyright (c) 2019 kildom
 * All rights reserved
 */

#include <stdint.h>
#include <string.h>

#include "md5.h"

static const uint8_t s[] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

static const uint32_t K[] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

static void md5_calc_chunk(struct md5 *ctx)
{
    int i;
    int g;
    uint32_t F;
    uint32_t *M = ctx->chunk;
    uint32_t A = ctx->A;
    uint32_t B = ctx->B;
    uint32_t C = ctx->C;
    uint32_t D = ctx->D;
    for (i = 0; i < 64; i++)
    {
        if (i <= 15)
        {
            F = (B & C) | (D & ~B);
            g = i;
        }
        else if (i <= 31)
        {
            F = (D & B) | (C & ~D);
            g = 5 * i + 1;
        }
        else if (i <= 47)
        {
            F = B ^ C ^ D;
            g = 3 * i + 5;
        }
        else
        {
            F = C ^ (B | ~D);
            g = 7 * i;
        }
        F = F + A + K[i] + M[g & 15];
        A = D;
        D = C;
        C = B;
        B = B + ((F << s[i]) | (F >> (32 - s[i])));
    }
    ctx->A += A;
    ctx->B += B;
    ctx->C += C;
    ctx->D += D;
}

void md5_init(struct md5 *ctx)
{
    ctx->A = 0x67452301;
    ctx->B = 0xefcdab89;
    ctx->C = 0x98badcfe;
    ctx->D = 0x10325476;
    ctx->length = 0;
    ctx->chunk_length = 0;
}

void md5_update(struct md5 *ctx, const unsigned char *data, int length)
{
    uint8_t *chunk_bytes = (uint8_t *)ctx->chunk;
    ctx->length += length;
    while (length > 0)
    {
        int part = length;
        if (part > sizeof(ctx->chunk) - ctx->chunk_length)
            part = sizeof(ctx->chunk) - ctx->chunk_length;
        memcpy(&chunk_bytes[ctx->chunk_length], data, part);
        ctx->chunk_length += part;
        length -= part;
        data += part;
        if (ctx->chunk_length == sizeof(ctx->chunk))
        {
            md5_calc_chunk(ctx);
            ctx->chunk_length = 0;
        }
    }
}

void md5_finalize(struct md5 *ctx, unsigned char *hash)
{
    uint8_t *chunk_bytes = (uint8_t *)ctx->chunk;
    memset(&chunk_bytes[ctx->chunk_length], 0, sizeof(ctx->chunk) - ctx->chunk_length);
    chunk_bytes[ctx->chunk_length] = 0x80;
    ctx->chunk_length++;
    if (ctx->chunk_length > sizeof(ctx->chunk) - sizeof(ctx->length))
    {
        md5_calc_chunk(ctx);
        memset(chunk_bytes, 0, sizeof(ctx->chunk));
    }
    ctx->length *= 8;
    memcpy(&chunk_bytes[sizeof(ctx->chunk) - sizeof(ctx->length)], &ctx->length, sizeof(ctx->length));
    md5_calc_chunk(ctx);
    memcpy(hash, &ctx->A, 4);
    memcpy(&hash[4], &ctx->B, 4);
    memcpy(&hash[8], &ctx->C, 4);
    memcpy(&hash[12], &ctx->D, 4);
}

void md5_calc(const unsigned char *data, int length, unsigned char *hash)
{
    struct md5 ctx;
    md5_init(&ctx);
    md5_update(&ctx, data, length);
    md5_finalize(&ctx, hash);
}
