struct A
{
	int i;
	struct B
	{
		int x, y;
	} b;
	int j;
} a = {
	.j = 1,
	.b.x = 2,
	3 // b.y
};
