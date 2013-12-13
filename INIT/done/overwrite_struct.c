struct A
{
	struct Sub
	{
		int i, j;
	} sub;
} x = {
	.sub.i = 1,
	.sub.j = 2,
};
