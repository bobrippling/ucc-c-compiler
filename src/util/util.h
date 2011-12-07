#ifndef UTIL_H
#define UTIL_H

typedef struct where
{
	const char *fname;
	int line, chr;
} where;

const char *where_str(const struct where *w);

void warn_at(struct where *, const char *, ...);
void die_at( struct where *, const char *, ...);
void vwarn(  struct where *, const char *, va_list);
void vdie(   struct where *, const char *, va_list);

void die(const char *fmt, ...);

void die_ice(const char *, int);
char *fline(FILE *f);
char *udirname(const char *);
char *ext_replace(const char *str, const char *ext);

void dynarray_add(void ***, void *);
int  dynarray_count(void ***);
void dynarray_free(void ***par, void (*f)(void *));

void ice(const char *f, int line, const char *fmt, ...);
void icw(const char *f, int line, const char *fmt, ...);
#define UCC_ASSERT(b, ...) do{ if(!(b)) ice(__FILE__, __LINE__, __VA_ARGS__); } while(0)
#define ICE(...) ice(__FILE__, __LINE__, __VA_ARGS__)
#define ICW(...) icw(__FILE__, __LINE__, __VA_ARGS__)

#endif
