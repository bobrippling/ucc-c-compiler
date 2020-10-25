// RUN: %check -e %s

struct A
{
	struct B
	{
		int i, j;
	} b, c;
};

f(struct B *b)
{
	struct A fine = {
		.b = *b // CHECK: !/error/
	};

	struct A bad1 = {
		{ /*b::i = */ *b } // CHECK: /error: mismatching types, init/
		// CHECK: ^ note: 'int' vs 'struct B'
	};
	struct A bad2 = {
		.b = { *b } // CHECK: /error: mismatching types, init/
		// CHECK: ^ note: 'int' vs 'struct B'
	};
	struct A bad3 = {
		*b // CHECK: !/error/
	};
}
