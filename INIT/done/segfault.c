struct {
	int a[5], b;
} game[] = {
	[0].a = { 1 },
	[1].a[0] = 2
};

struct { int a[5], b; } game2[] = {
	{ 1 },
	2
};
