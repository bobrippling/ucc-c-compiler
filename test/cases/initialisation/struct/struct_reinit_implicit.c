// RUN: %layout_check %s
struct A
{
	int x, y, z;
} ent1 = {
	.y = 5,
	.x = 2,
	1,
	3
};
