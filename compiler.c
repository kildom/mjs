/*
 * Copyright (c) 2019 kildom
 * All rights reserved
 */


#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mjs/src/mjs_core_public.h"
#include "mjs/src/mjs_core.h"
#include "mjs/src/mjs_exec.h"
#include "common/mbuf.h"
#include "common/cs_varint.h"
#include "md5.h"
#include "version.h"

#include "compiler.h"

#define INITIAL_BUFFER_SIZE (8 * 1024)

struct compiler
{
    struct mjs *mjs;
    struct mbuf buf;
    struct mbuf map_buf;
    unsigned char map_hash[16];
    int map_hash_calculated;
    size_t bytecode_offset;
};

struct compiler* c_create()
{
    struct compiler* c = NULL;

    c = malloc(sizeof(struct compiler));
    if (c == NULL) goto error_exit;
    memset(c, 0, sizeof(struct compiler));

    c->mjs = mjs_create();
    if (c->mjs == NULL) goto error_exit;

    mbuf_init(&c->buf, INITIAL_BUFFER_SIZE);
    if (c->buf.buf == NULL) goto error_exit;

    mbuf_init(&c->map_buf, 0);

    return c;

error_exit:
    if (c != NULL)
    {
        if (c->buf.buf != NULL)
            mbuf_free(&c->buf);
        if (c->mjs != NULL)
            mjs_destroy(c->mjs);
        free(c);
    }
    return NULL;
}

int c_begin(struct compiler* c, unsigned char** output, int* length)
{
    static const uint64_t ver = VERSION_LONG_LONG;
    *output = (unsigned char*)&ver;
    *length = sizeof(ver);
    return MJS_OK;
}

int c_compile(struct compiler* c, const char* source, const char* name, unsigned char** output, int* length)
{
    int res;
    const unsigned char* bytecode;
    int bytecode_len;
    size_t len;

    c->map_hash_calculated = 0;
    c->bytecode_offset = c->mjs->bcode_len;
    mbuf_free(&c->map_buf);

    res = mjs_compile(c->mjs, name, source, &bytecode, &bytecode_len, &c->map_buf);
    if (res != MJS_OK)
    {
        const char *error_string = mjs_strerror(c->mjs, res);
        *output = (unsigned char*)error_string;
        *length = strlen(error_string);
        return res;
    }

    len = 4 + 16 + bytecode_len;
    mbuf_resize(&c->buf, len);
    if (c->buf.size < len)
    {
        *output = "Out of memory";
        *length = strlen(*output);
        return MJS_OUT_OF_MEMORY;
    }

    memcpy(c->buf.buf, &len, 4);
    md5_calc(source, strlen(source), &c->buf.buf[4]);
    memcpy(&c->buf.buf[4 + 16], bytecode, bytecode_len);
    c->buf.len = 4 + 16 + bytecode_len;

    *output = c->buf.buf;
    *length = c->buf.len;
    return MJS_OK;
}


int c_get_source_hash(struct compiler* c, unsigned char** output, int* length)
{
    if (c->buf.len < 4 + 16)
    {
        *output = NULL;
        *length = 0;
        return MJS_BAD_ARGS_ERROR;
    }

    *output = &c->buf.buf[4];
    *length = 16;
    return MJS_OK;
}

int c_get_map_hash(struct compiler* c, unsigned char** output, int* length)
{
    if (!c->map_hash_calculated)
    {
        struct md5 hash_ctx;
        if (c->buf.len < 4 + 16)
        {
            *output = NULL;
            *length = 0;
            return MJS_BAD_ARGS_ERROR;
        }
        md5_init(&hash_ctx);
        md5_update(&hash_ctx, (unsigned char*)&c->bytecode_offset, 4);
        md5_update(&hash_ctx, &c->buf.buf[4], c->buf.len - 4);
        md5_finalize(&hash_ctx, c->map_hash);
        c->map_hash_calculated = 1;
    }
    *output = c->map_hash;
    *length = 16;
    return MJS_OK;
}

int c_get_map_data(struct compiler* c, unsigned char** output, int* length)
{
    *output = c->map_buf.buf;
    *length = c->map_buf.len;
    return MJS_OK;
}

int c_end(struct compiler* c, unsigned char** output, int* length)
{
    uint32_t len = 0;
    mbuf_clear(&c->buf);
    mbuf_append(&c->buf, &len, sizeof(len));
    *output = c->buf.buf;
    *length = c->buf.len;
    return MJS_OK;
}


void c_free(struct compiler* c)
{
    mbuf_free(&c->map_buf);
    mbuf_free(&c->buf);
    mjs_destroy(c->mjs);
}

