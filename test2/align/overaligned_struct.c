// RUN: %ucc -fsyntax-only -Wno-gnu-alignof-expression %s

#ifdef custom_offsetof
#  define offsetof(S, m) (long)&((S*)0)->m
#else
#  define offsetof __builtin_offsetof
#endif

struct __attribute((aligned(2)))  { char x[3]; } a_2;
_Static_assert(sizeof(a_2) == 4, "");
_Static_assert(_Alignof(a_2) == 2, "");

struct __attribute((aligned(4)))  { char x[3]; } a_4;
_Static_assert(sizeof(a_4) == 4, "");
_Static_assert(_Alignof(a_4) == 4, "");

struct __attribute((aligned(8)))  { char x[3]; } a_8;
_Static_assert(sizeof(a_8) == 8, "");
_Static_assert(_Alignof(a_8) == 8, "");

struct __attribute((aligned(16))) { int x;     } a_12;
_Static_assert(sizeof(a_12) == 16, "");
_Static_assert(_Alignof(a_12) == 16, "");


__attribute((aligned(8))) char c;
_Static_assert(sizeof(c) == sizeof(char), "");
_Static_assert(_Alignof(c) == 8, "");

// ------------------------

#ifdef __UCC__
#  define assert_is_attributed(exp, t) _Static_assert(exp == __builtin_has_attribute(struct t, aligned), "")
#else
#  define assert_is_attributed(exp, t)
#endif

struct bytes80 { char s[80]; };
assert_is_attributed(0, bytes80);
_Static_assert(sizeof(struct bytes80) == 80, "");

struct bytes80_and_int { struct bytes80 f; int i; };
assert_is_attributed(0, bytes80_and_int);
_Static_assert(sizeof(struct bytes80_and_int) == 80 + sizeof(int), "");
_Static_assert(offsetof(struct bytes80_and_int, i) == 80, "");

struct __attribute((aligned(64))) bytes80_aligned { char s[80]; };
assert_is_attributed(1, bytes80_aligned);
_Static_assert(sizeof(struct bytes80_aligned) == 128, "");
_Static_assert(sizeof(struct bytes80_aligned) == 128, "");

struct bytes80_aligned_and_int { struct bytes80_aligned f; int i; };
assert_is_attributed(0, bytes80_aligned_and_int);
_Static_assert(sizeof(struct bytes80_aligned_and_int) == 192, "");
_Static_assert(offsetof(struct bytes80_aligned_and_int, i) == 128, "");
