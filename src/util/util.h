#ifndef UTIL_H
#define UTIL_H

typedef struct where
{
	const char *fname;
	int line, chr;
} where;

void die_at(struct where *, const char *, ...);
void vdie(struct where *w, va_list l, const char *fmt);
void die(const char *, ...);
void die_ice(const char *, int);
char *fline(FILE *f);
void dynarray_add(void ***, void *);

#define DIE_ICE() die_ice(__FILE__, __LINE__)

#endif
