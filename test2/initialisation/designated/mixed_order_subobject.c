// RUN: %layout_check %s
int x[][2] = {
	[1][0] = 2,
	[0][0] = 0,
	[1][1] = 3,
	[0][1] = 1,
};

struct A
{
	struct B
	{
		int i, j;
	} a;
} y = {
	.a.i = 1,
	.a.j = 2,
};
