// RUN: %layout_check %s
struct A
{
	int i;
	struct B
	{
		int x, y, z;
	} b;
	int sub_ar[3];
	int j;
} ent1[] = {
	[2] = 1,
	[3].j = 2,
	3
};
