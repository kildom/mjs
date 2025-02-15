/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_PARSER_H
#define MJS_PARSER_H

#include "common/mbuf.h"
#include "mjs/src/mjs_internal.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

MJS_PRIVATE mjs_err_t
mjs_parse(const char *path, const char *buf, struct mjs *, struct mbuf* line_no_map);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* MJS_PARSER_H */
