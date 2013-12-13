#ifndef COMPILER_H
#define COMPILER_H

#ifdef __GNUC__

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

#endif
