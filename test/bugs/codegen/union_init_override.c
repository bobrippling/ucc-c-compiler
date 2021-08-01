union
{
	struct
	{
		int x, y;
	} p;

	int q;
} a = {
	{
		1,
		2
	},
	.q = 3
};
