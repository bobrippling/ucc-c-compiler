// RUN: %layout_check %s
struct A
{
	int i;
	struct B
	{
		int x, y;
	} b;
	int j;
} ent1 = {
	.j = 1,
	.b.x = 2,
	3 // b.y
};
