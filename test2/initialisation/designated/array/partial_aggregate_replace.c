// RUN: %layout_check %s

struct { int i; } ent[] = {
	[10] = 3,
	[0 ... 5] = 2,
	[0] = { 1 },
	[3] = 5,
};
// 1, 2, 2, 5, 2, 2
