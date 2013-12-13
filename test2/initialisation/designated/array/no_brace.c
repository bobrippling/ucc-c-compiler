// RUN: %layout_check %s

int a[][2] = {
	[1] = 1, // [1][0]
	2,       // [1][1]
	3        // [2][0]
};

// 24 / sizeof(int[2]) = 3
int const K = sizeof(a)/sizeof(*a);
