#ifndef UTIL_H
#define UTIL_H

void die_at(const char *, ...);
void die(const char *, ...);
char *fline(FILE *f);
void dynarray_add(void ***, void *);

#endif
