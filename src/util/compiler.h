#ifndef COMPILER_H
#define COMPILER_H

#if __STDC__ >= 201112L
#  define ucc_noreturn _Noreturn
#  define ucc_static_assert(tag, test) _Static_assert(test, #test " (" #tag ")")
#endif

#ifdef __GNUC__

#  define ucc_printflike(fmtarg, firstvararg) \
			__attribute__((__format__ (__printf__, fmtarg, firstvararg)))

#  define ucc_dead __attribute__((noreturn))
#  define ucc_wur  __attribute__((warn_unused_result))
#  define ucc_nonnull(args) __attribute__((nonnull args))
#  define ucc_static_param static
#  define ucc_const __attribute__((const))
#  define ucc_unused __attribute__((unused))
#  ifndef ucc_noreturn
#    define ucc_noreturn __attribute__((noreturn))
#  endif

#else
#  define ucc_printflike(a, b)
#  define ucc_dead
#  define ucc_wur
#  define ucc_nonnull(a)
#  define ucc_static_param
#  define ucc_const
#  define ucc_unused
#endif

#ifndef ucc_noreturn
#  define ucc_noreturn
#endif

#ifndef ucc_static_assert
#  define ucc_static_assert(tag, test) typedef char check_ ## tag[(test) ? 1 : -1]
#endif

#define memcpy_safe(a, b) (*(a) = *(b))
#define REMOVE_CONST(t, exp) ((t)(exp))

#ifdef __UCC__
#  define COMPILER_SUPPORTS_LONG_DOUBLE 0
#else
#  define COMPILER_SUPPORTS_LONG_DOUBLE 1
#endif

#endif
