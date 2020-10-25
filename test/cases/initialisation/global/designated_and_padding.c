// RUN: %layout_check %s
struct
{
	int i, j, k[2];
} x[] = {
	//1, 2, 3, 4,
	//{ 5, 6, 7, 8 },
	//{ 9, 10, { 11, 12 } },
	{ { 13 }, 14, { 15 } },
	{ .k[1] = 9 },
};

struct A { int i, j; } a, b = { 1 };

struct A y[10];
