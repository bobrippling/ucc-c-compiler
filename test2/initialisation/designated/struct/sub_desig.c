struct A
{
	struct B
	{
		int i, j;
	} b;
} a = {
	.b.j = 5
};
