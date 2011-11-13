#ifndef UTIL_H
#define UTIL_H

typedef struct where
{
	const char *fname;
	int line, chr;
} where;

const char *where_str(const struct where *w);

void warn_at(struct where *, const char *, ...);
void die_at(struct where *, const char *, ...);
void vdie(struct where *w, va_list l, const char *fmt);
void die(const char *fmt, ...);
void die_ice(const char *, int);
char *fline(FILE *f);
char *udirname(const char *);

void dynarray_add(void ***, void *);
int  dynarray_count(void ***);
int  dynarray_free(void ***, void (*)(void *));

void ice(const char *f, int line, const char *fmt, ...);
void icw(const char *f, int line, const char *fmt, ...);
#define ICE(...) ice(__FILE__, __LINE__, __VA_ARGS__)
#define ICW(...) icw(__FILE__, __LINE__, __VA_ARGS__)

#endif
