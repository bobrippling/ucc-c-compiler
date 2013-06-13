// RUN: %layout_check %s
struct
{
	int a, b;
} sigs[] = {
	[0 ... 2] = { 1, 2 },
	[0].a = 99
};
