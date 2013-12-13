// RUN: %layout_check %s

struct A
{
	int a;
	struct B
	{
		int i, j;
	} b;
	int c;
} x = {
	.b = 2, // b.i
	3, // b.j
	.c = 4
};
