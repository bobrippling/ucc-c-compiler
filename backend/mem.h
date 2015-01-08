#ifndef MEM_H
#define MEM_H

#include <stddef.h>

void *xmalloc(size_t);
void *xcalloc(size_t, size_t);
char *xstrdup(const char *);

#endif
