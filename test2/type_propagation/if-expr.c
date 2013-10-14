// RUN: %ucc -fsyntax-only %s

/* 0 or (void *)0 is a null pointer constant.
 * anything else is not.
 */

#define TY_EQ(a, b) _Static_assert(__builtin_types_compatible_p(a, b), "mismatch")

TY_EQ(int  *, typeof(1 ? (int *)0 : (void *)0)); // null pointer constant -> other operand's type
TY_EQ(void *, typeof(1 ? (int *)0 : (void *)1)); // neither n-p-c, types go to void *
