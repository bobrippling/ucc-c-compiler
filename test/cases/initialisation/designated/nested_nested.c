// RUN: %layout_check %s

struct A
{
	struct B
	{
		struct C
		{
			struct
			{
				int i, j;
			} j, k;
		} c;
	} b;
} a = {
	.b.c = {
		.j.j = 1,
		.k.j = 3
	}
};
