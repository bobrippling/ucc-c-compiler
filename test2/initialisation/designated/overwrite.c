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
	1,
	.b.c.i = 2,
	3
};

int b[][2] = {
	[1][0] = 2,
	[0][1] = 0,
	[1][1] = 3,
	[0][1] = 1,
};

struct { int x, y; } c[] = {
	[2].x = 1,
	[2]   = 2,
};
