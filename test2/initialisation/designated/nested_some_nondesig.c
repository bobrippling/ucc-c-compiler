// RUN: %layout_check %s

int ent2[][2] = {
	[1][1] = {
		1,
		2
	}
};

// 0,0,
// 0,1

struct { int i, j, k, l; } ent3[][2] = {
	[1][1] = {
		1,
		.k = 2,
		3
	}
};

// 0,0,0,0,
// 0,0,0,0,
// 0,0,0,0,
// 1,0,2,3

struct { int i, j; } abc[] = {
	[1].i = {
		1,
		2
	}
};

// 0,0
// 1,2 // should perhaps be 1,0
