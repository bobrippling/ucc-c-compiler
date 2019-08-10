// RUN: %layout_check %s
struct A
{
	struct B
	{
		struct C
		{
			int i, j, k;
		} c;
	} b;
} a = {
	1, // pass this and the next through to the sub-init,
	2, // but nothing after the desig
	.b.c.j = 5,
	3
};
