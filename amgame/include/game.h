#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

void init_video();
const char* read_keys();
void draw_mario(int, int, int);

static inline void puts(const char *s) {
  for (; *s; s++) _putc(*s);
}
