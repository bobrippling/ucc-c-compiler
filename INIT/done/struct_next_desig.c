struct A
{
	struct
	{
		int i, j;
	} x, y;
} a = {
	1, // if next is a designator, don't pass it down
	.x.j = 3
};
