#ifndef UTIL_H
#define UTIL_H

#if defined(__GNUC__)

#  define ucc_printflike(fmtarg, firstvararg) \
			__attribute__((__format__ (__printf__, fmtarg, firstvararg)))

#  define ucc_dead __attribute__((noreturn))
#  define ucc_wur  __attribute__((warn_unused_result))
#  define ucc_nonnull(args) __attribute__((nonnull args))
#  define ucc_static_param static
#  define ucc_const __attribute__((const))

#else
#  define ucc_printflike(a, b)
#  define ucc_dead
#  define ucc_wur
#  define ucc_nonnull(a)
#  define ucc_static_param
#endif

#define memcpy_safe(a, b) (*(a) = *(b))

#define ucc_unreach() do{ ICE("unreachable"); return 0; }while(0)

typedef struct where
{
	const char *fname, *line_str;
	int line, chr;
} where;

#define WHERE_BUF_SIZ 128
const char *where_str(const struct where *w);
const char *where_str_r(char buf[ucc_static_param WHERE_BUF_SIZ], const struct where *w);

void warn_at(const struct where *, int show_line, const char *, ...) ucc_printflike(3, 4);
void die_at( const struct where *, int show_line, const char *, ...) ucc_printflike(3, 4) ucc_dead;
void vwarn(const struct where *w, int err, int show_line, const char *fmt, va_list l);
void vdie(   const struct where *, int show_line, const char *, va_list) ucc_dead;
void die(const char *fmt, ...) ucc_printflike(1, 2) ucc_dead;

#define DIE_AT( w, ...) die_at( w, 1, __VA_ARGS__)
#define WARN_AT(w, ...) warn_at(w, 1, __VA_ARGS__)

char *fline(FILE *f);
char *udirname(const char *);
char *ext_replace(const char *str, const char *ext);

void ice(const char *f, int line, const char *fn, const char *fmt, ...) ucc_printflike(4, 5) ucc_dead;
void icw(const char *f, int line, const char *fn, const char *fmt, ...) ucc_printflike(4, 5);
#define UCC_ASSERT(b, ...) do if(!(b)) ICE(__VA_ARGS__); while(0)
#define ICE(...) ice(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define ICW(...) icw(__FILE__, __LINE__, __func__, __VA_ARGS__)

char *terminating_quote(char *);

#endif
