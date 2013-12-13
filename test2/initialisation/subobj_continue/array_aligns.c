// RUN: %layout_check %s

struct A
{
	int x[8], i, y[4];
} abc = {
	.x[2 /*... 3*/] = 76,
	3,
	4,
	5,
	.i = 3//[0 /*... 3*/] = 93
	// space of 16 at the end
};
