#ifndef SHARED_COMPILER_H
#define SHARED_COMPILER_H

#define countof(x) (sizeof(x) / sizeof*(x))

#ifdef __GNUC__
#  define compiler_printf(x, y) __attribute__((format(printf, x, y)))
#  define compiler_warn_unused __attribute__((warn_unused_result))
#  define compiler_nonnull(...) __attribute__((nonnull __VA_ARGS__))
#  define compiler_static static
#else
#  define compiler_printf(x, y)
#  define compiler_warn_unused
#  define compiler_nonnull(...)
#  define compiler_static
#endif

#endif
