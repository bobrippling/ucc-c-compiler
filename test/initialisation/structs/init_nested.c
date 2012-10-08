struct Outer
{
	struct Inner
	{
		int i, j;
	} a;

	int k;
	char c;

	struct { int a, b; struct { char *p, c; } q; } i;

} b[] = {
	{
		{ 5, 3 },
		//2
	},
	{
		{ 1, 2 },
		3
	}
};
