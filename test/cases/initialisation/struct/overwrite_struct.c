// RUN: %layout_check %s
struct A
{
	struct Sub
	{
		int i, j;
	} sub;
} ent1 = {
	.sub.i = 1,
	.sub.j = 2,
};
