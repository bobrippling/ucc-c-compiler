/*char x[3] = {
	[5] = 2
};*/

struct A
{
	struct B
	{
		int i;
	} b;
	int j;
} a = { .b.i = 2 };
