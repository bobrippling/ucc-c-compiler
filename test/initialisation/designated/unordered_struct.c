// RUN: %check %s
// RUN: %layout_check %s

struct
{
	int i;
	int j;
	int k;
} x = {
	.j = 1,
	2,
	3, // CHECK: /warning: excess/
};
