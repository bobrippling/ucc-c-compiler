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
	{ 1 },
	2, 3,4,5, 6,7,8, 9,
	{},
	8,9,10, {1}, 11, 12, {13}, {5},
	14
};
