// RUN: %check %s
// types should propagate, ignoring the struct
struct A
{
	int i, j;
};
int x = __builtin_choose_expr(0, (struct A){ 1, 2 }, 3); // CHECK: !/warn/
