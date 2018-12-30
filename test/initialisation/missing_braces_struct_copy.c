// RUN: %check %s

struct A
{
	struct B
	{
		int i, j;
	} b;
};

f(struct B *b)
{
	struct A fine = {
		.b = *b // CHECK: !/missing/
	};

	struct A warn = {
		1, 2 // CHECK: /warning: missing braces/
	};
}
