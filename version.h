/*
 * Copyright (c) 2019 kildom
 * All rights reserved
 */

#ifndef version_h_
#define version_h_

#ifndef COMMIT_HASH
#define COMMIT_HASH 0000000000000000
#warning COMMIT_HASH is not defined
#endif

#define _VERSION_CONCAT1(a, b, c) a##b##c
#define _VERSION_CONCAT2(a, b, c) _VERSION_CONCAT1(a, b, c)
#define _VERSION_SWAP(val) ( \
    (((val) >> 56) & 0x00000000000000FFuLL) | \
    (((val) >> 40) & 0x000000000000FF00uLL) | \
    (((val) >> 24) & 0x0000000000FF0000uLL) | \
    (((val) >>  8) & 0x00000000FF000000uLL) | \
    (((val) <<  8) & 0x000000FF00000000uLL) | \
    (((val) << 24) & 0x0000FF0000000000uLL) | \
    (((val) << 40) & 0x00FF000000000000uLL) | \
    (((val) << 56) & 0xFF00000000000000uLL) )

#define VERSION_LONG_LONG_INV _VERSION_CONCAT2(0x, COMMIT_HASH, uLL)
#define VERSION_LONG_LONG _VERSION_SWAP(VERSION_LONG_LONG_INV)

#endif
