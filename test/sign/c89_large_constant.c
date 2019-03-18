// RUN: %ucc -fsyntax-only %s -std=c89 -target i386-linux
// RUN: %ucc -fsyntax-only %s -std=c99 -target i386-linux

_Static_assert(sizeof(long) == 4, "need to be 32-bit");

__auto_type x = 2147483648;
__auto_type y = 2147483649;

/* C90 = unsigned long */
/* C99 = long long */

#ifdef __STDC_VERSION__
typedef long long expected;
#else
typedef unsigned long expected;
#endif

_Static_assert(_Generic(x, expected: 1) == 1, "");
_Static_assert(_Generic(y, expected: 1) == 1, "");
