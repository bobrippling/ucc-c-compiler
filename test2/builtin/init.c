// RUN: %ucc -S -o %t %s

void f()
{
	// this tests that we're able to run things such as malloc checks in
	// assignments, since __builtin_choose_expr() isn't an lvalue, but should
	// pass the various assumptions in type checks
	int x = __builtin_choose_expr(1, 1, 1);
}
