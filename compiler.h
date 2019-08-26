/*
 * Copyright (c) 2019 kildom
 * All rights reserved
 */

#ifndef COMPILER_H_
#define COMPILER_H_

struct compiler;

struct compiler* c_create();
int c_begin(struct compiler* c, unsigned char** output, int* length);
int c_compile(struct compiler* c, const char* source, const char* name, unsigned char** output, int* length);
int c_get_source_hash(struct compiler* c, unsigned char** output, int* length);
int c_get_map_hash(struct compiler* c, unsigned char** output, int* length);
int c_get_map_data(struct compiler* c, unsigned char** output, int* length);
int c_end(struct compiler* c, unsigned char** output, int* length);
void c_free(struct compiler* c);


#endif
