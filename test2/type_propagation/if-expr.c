// RUN: %ucc -fsyntax-only %s

/* 0 or (void *)0 is a null pointer constant.
 * anything else is not.
 */

#define TY_EQ(ty, exp) _Static_assert(_Generic(exp, ty: 1) == 1, "mismatch")

TY_EQ(int  *, 1 ? (int *)0 : (void *)0); // null pointer constant -> other operand's type
TY_EQ(void *, 1 ? (int *)0 : (void *)1); // neither n-p-c, types go to void *
