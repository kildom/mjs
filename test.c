
#include <stdio.h>
#include <string.h>
#include "mjs/src/mjs_core_public.h"
#include "mjs/src/mjs_ffi_public.h"

void foo(int x) {
  printf("Hello %d!\n", x);
}

void *my_dlsym(void *handle, const char *name) {
  if (strcmp(name, "foo") == 0) return foo;
  return NULL;
}

mjs_err_t mjs_compile(struct mjs *mjs, const char *path, const char *src, const char **p, size_t *len);
mjs_err_t mjs_exec_compiled(struct mjs *mjs, const char *p, size_t len, mjs_val_t *res);

int main(void) {
  struct mjs *mjs = mjs_create();
  mjs_set_ffi_resolver(mjs, my_dlsym);
  const char *p = "";
  size_t len = 0;
  size_t i;
  char temp[65536];
  FILE* f = fopen("a.js", "rb");
  size_t s = fread(temp, 1, sizeof(temp) - 1, f);
  fclose(f);
  if (s <= 0) return 1;
  temp[s] = 0;
  #ifdef WITH_PARSER
  //mjs_compile(mjs, "test.js", "let f = ffi('void foo(int)'); f(123)", &p, &len);
  mjs_compile(mjs, "test.js", temp, &p, &len);
  f = fopen("a.jsc", "wb");
  fwrite(p, 1, len, f);
  fclose(f);
  #endif
  mjs_destroy(mjs);

  f = fopen("a.jsc", "rb");
  len = fread(temp, 1, sizeof(temp), f);
  p = temp;
  fclose(f);

  printf("%d\n", len);
  for (i = 0; i < len; i++)
  {
      if (i > 0 && (i % 16) == 0) printf("\n");
      printf("%02X ", (uint32_t)p[i] & 0xFF);
  }
  printf("\n");


  struct mjs *mjs2 = mjs_create();
  mjs_set_ffi_resolver(mjs2, my_dlsym);

  mjs_exec_compiled(mjs2, p, len, NULL);

  mjs_destroy(mjs2);

  return 0;
}