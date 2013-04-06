// RUN: %ucc -c %s
// RUN: %check %s
// RUN: %asmcheck %s

struct
{
	int i;
	int j;
	int k;
} x = {
	.j = 1,
	2,
	3, // CHECK: /warning: excess struct initialiser/
};
