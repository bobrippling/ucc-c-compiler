// RUN: %check --prefix=norm %s -Wall -Wextra
// RUN: %check --prefix=with-warning %s -Wnull-zero-literal

f(int *p)
{
	return p == 0; // CHECK-norm: !/warning/
	// CHECK-with-warning: ^ warning: mismatching types, comparison between pointer and integer
}

g(int *p)
{
	return 0 == p; // CHECK-norm: !/warning/
	// CHECK-with-warning: ^ warning: mismatching types, comparison between pointer and integer
}

enum { ZERO };

h(int *p)
{
	return p == ZERO; // CHECK-norm: !/warning/
	// CHECK-with-warning: ^ warning: mismatching types, comparison between pointer and integer
}

i(int *p)
{
	return p == (1-1); // CHECK-norm: !/warning/
	// CHECK-with-warning: ^ warning: mismatching types, comparison between pointer and integer
}
