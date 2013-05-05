// RUN: %asmcheck %s

int a[][2] = {
	[1] = 1, // [1][0]
	2,       // [2][0]
	3        // [2][0]
};

int const K = sizeof(a)/sizeof(*a);
