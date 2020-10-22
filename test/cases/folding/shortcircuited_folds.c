// RUN: %check -e --only %s -w

const struct B { int x; } b = { 1 };

/* expression where one side is invalid - ensure the other sides are still folded/type-checked */

int *p1 = &b[3]; /* op: *(&b + 3) */
// CHECK: ^error: struct involved in subscript
// CHECK: ^^error: indirection applied to 'int'
// CHECK: ^^^error: global scalar initialiser not constant

int *p2 = b[3] ? 1 : 2;
// CHECK: ^error: struct involved in subscript
// CHECK: ^^error: indirection applied to 'int'
// CHECK: ^^^error: global scalar initialiser not constant
