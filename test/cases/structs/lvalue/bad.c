// RUN: %check -e %s

typedef struct B {
	long a,b,c,d;
} B;

B f();

main()
{
	struct A
	{
		int i;
	} a, b, c;

	a = b ? : c; // CHECK: error: struct involved in ?:

	(a = b) = c; // CHECK: error: assignment to struct A - not an lvalue

	f(3).a = 1; // CHECK: error: assignment to long - not an lvalue
	(a = b).i = 1; // CHECK: error: assignment to int - not an lvalue
	(a.i ? a : b).i = 1; // CHECK: error: assignment to int - not an lvalue
	(0, a).i = 1; // CHECK: error: assignment to int - not an lvalue
}
