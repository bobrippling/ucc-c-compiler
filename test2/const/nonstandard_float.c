// RUN: %check %s

/* not a constant expression
 * C11, 6.6/6:
 * An integer constant expression117) shall have integer type and shall only
 * have operands that are integer constants, enumeration constants, character
 * constants, sizeof expressions whose results are integer constants, _Alignof
 * expressions, and floating constants that are the immediate operands of
 * casts. Cast operators in an integer constant expression shall only convert
 * arithmetic types to integer types, except as part of an operand to the
 * sizeof or _Alignof operator.
 */

int x[(int)(1.2 + 3.2)]; // CHECK: /warning: expr-op is non-standard/
//              ^ addition of non-ints