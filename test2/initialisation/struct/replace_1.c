// RUN: %layout_check %s
struct
{
	int a, b;
} sigs[] = {
	[0 ... 5] = { 5, 6 },
	[1].b = 2, // or .a
};
