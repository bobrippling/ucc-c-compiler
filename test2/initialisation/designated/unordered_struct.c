// RUN: %ucc -c %s
// RUN: %ucc -c %s 2>&1 | %check %s
// RUN: %ucc -S -o- %s | %asmcheck %s

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
