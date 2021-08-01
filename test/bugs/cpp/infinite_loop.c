#define __GNUC_PREREQ__(...) 1

#if (__STDC_VERSION__ >= 201112L)
#define __fp_type_select(x, f, d, ld) _Generic((x),                     \
    short: f(x),                                                        \
    int: d(x),                                                       \
    long int: ld(x),                                                 \
    volatile short: f(x),                                               \
    volatile int: d(x),                                              \
    volatile long int: ld(x),                                        \
    volatile const short: f(x),                                         \
    volatile const int: d(x),                                        \
    volatile const long int: ld(x),                                  \
    const short: f(x),                                                  \
    const int: d(x),                                                 \
    const long int: ld(x))
#elif __GNUC_PREREQ__(3, 1) && !defined(__cplusplus)
#define __fp_type_select(x, f, d, ld) __builtin_choose_expr(            \
    __builtin_types_compatible_p(__typeof(x), long int), ld(x),      \
    __builtin_choose_expr(                                              \
    __builtin_types_compatible_p(__typeof(x), int), d(x),            \
    __builtin_choose_expr(                                              \
    __builtin_types_compatible_p(__typeof(x), short), f(x), (void)0)))
#else
#define  __fp_type_select(x, f, d, ld)                                  \
    ((sizeof(x) == sizeof(short)) ? f(x)                                \
    : (sizeof(x) == sizeof(int)) ? d(x)                              \
    : ld(x))
#endif

#define iszero(x) \
        __fp_type_select(x, iszeros, iszero, iszerol)

int iszeros(short x) { return x==0; }
int iszero(int x) { return x==0; }
int iszerol(long x) { return x==0; }

int
foo(int x)
{
   int y = 0;
   if (iszero(x)) y = 42;
   return (y);
}
