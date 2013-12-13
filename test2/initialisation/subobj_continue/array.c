// RUN: %layout_check %s

struct A
{
	int a;
	int x[4];
	int c;
} x = {
	.x = 2,
	3, // x[1]
	.c = 4
};
