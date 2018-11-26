// RUN: %ucc -E -o /dev/null %s

#if !__has_builtin(__builtin_constant_p)
#error x
#endif

#if __has_builtin(constant_p)
#error x
#endif
