#ifndef UTIL_MACROS_H
#define UTIL_MACROS_H

#ifndef MIN
#  define MIN(x, y) ((x) < (y) ? (x) : (y))
#  define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#define countof(ar) (sizeof(ar) / sizeof((ar)[0]))

#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)

#define JOIN_(a, b) a ## b
#define JOIN(a, b) JOIN_(a, b)

#ifdef NDEBUG
#  define UCC_DEBUG_BUILD 0
#else
#  define UCC_DEBUG_BUILD 1
#endif

#endif
