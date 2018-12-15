// RUN: %layout_check %s

struct A
{
	int i, j;
} x = {
	.j = 1,
	.i = 2
};
