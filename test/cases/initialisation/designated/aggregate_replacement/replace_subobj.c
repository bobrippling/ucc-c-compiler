// RUN: %layout_check %s

struct
{
	int i, j;
} x[] = {
	[0 ... 5] = { 1, 2 },
	[2].j = 7
};
