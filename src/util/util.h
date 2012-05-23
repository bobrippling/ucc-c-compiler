#ifndef UTIL_H
#define UTIL_H

#if __GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ >= 7
#  define __printflike(fmtarg, firstvararg) \
			__attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#else
#  define __printflike(a, b)
#endif

typedef struct where
{
	const char *fname;
	int line, chr;
} where;

const char *where_str(const struct where *w);

void warn_at(struct where *, const char *, ...) __printflike(2, 3);
void die_at( struct where *, const char *, ...) __printflike(2, 3);
void vwarn(struct where *w, int err, const char *fmt, va_list l);
void vdie(   struct where *, const char *, va_list);
void die(const char *fmt, ...) __printflike(1, 2);

char *fline(FILE *f);
char *udirname(const char *);
char *ext_replace(const char *str, const char *ext);

void ice(const char *f, int line, const char *fn, const char *fmt, ...) __printflike(4, 5);
void icw(const char *f, int line, const char *fn, const char *fmt, ...) __printflike(4, 5);
#define UCC_ASSERT(b, ...) do{ if(!(b)) ICE(__VA_ARGS__); } while(0)
#define ICE(...) ice(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define ICW(...) icw(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define WHERE_BUF_SIZ 128

#endif
