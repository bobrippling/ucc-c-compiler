#ifndef UTIL_H
#define UTIL_H

void die_at(struct where *, const char *, ...);
void die(const char *, ...);
void die_ice(const char *, int);
char *fline(FILE *f);
void dynarray_add(void ***, void *);

#define DIE_ICE() die_ice(__FILE__, __LINE__)

#endif
