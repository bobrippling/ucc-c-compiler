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
void vwarn(struct where *w, int err, const char *fmt, va_list l);
void vdie(   struct where *, const char *, va_list);
void die(const char *fmt, ...);

char *fline(FILE *f);
char *udirname(const char *);
char *ext_replace(const char *str, const char *ext);

void ice(const char *f, int line, const char *fn, const char *fmt, ...);
void icw(const char *f, int line, const char *fn, const char *fmt, ...);
#define UCC_ASSERT(b, ...) do{ if(!(b)) ICE(__VA_ARGS__); } while(0)
#define ICE(...) ice(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define ICW(...) icw(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define WHERE_BUF_SIZ 128

#endif
