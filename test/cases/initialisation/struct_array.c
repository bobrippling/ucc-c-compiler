// RUN: %layout_check %s
struct {
	int a[5], b;
} ent1[] = {
	[0].a = { 1 },
	[1].a[0] = 2
};

struct { int a[5], b; } ent2[] = {
	{ 1 },
	2
};
