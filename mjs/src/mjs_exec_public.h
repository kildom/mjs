/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_EXEC_PUBLIC_H_
#define MJS_EXEC_PUBLIC_H_

#include "mjs/src/mjs_core_public.h"
#include "common/mbuf.h"
#if CS_ENABLE_STDIO
#include <stdio.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

mjs_err_t mjs_exec(struct mjs *, const char *src, mjs_val_t *res);
mjs_err_t mjs_exec_buf(struct mjs *, const char *src, size_t, mjs_val_t *res);

mjs_err_t mjs_exec_file(struct mjs *mjs, const char *path, mjs_val_t *res);
mjs_err_t mjs_apply(struct mjs *mjs, mjs_val_t *res, mjs_val_t func,
                    mjs_val_t this_val, int nargs, mjs_val_t *args);
mjs_err_t mjs_call(struct mjs *mjs, mjs_val_t *res, mjs_val_t func,
                   mjs_val_t this_val, int nargs, ...);
mjs_val_t mjs_get_this(struct mjs *mjs);

mjs_err_t mjs_compile(struct mjs *mjs, const char *path, const char *src, const unsigned char **p, size_t *len, struct mbuf* line_no_map);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* MJS_EXEC_PUBLIC_H_ */
