int ent1[][2] = {
	[3][1] = 3,
	[5][0] = {{{ 2 }}},
	[2] = 2,
	[4] = { 1, 2 },
	[1][1] = { 1, 2, 3 },
};

/*
0, 0,
0, 1,
2, 0,
0, 3,
1, 2,
2, 0
*/

int ent2[][2] = {
	// this checks we pass the second [1] designator to:
	// { [1] = 1, 2, 3 }
	[1][1] = { 1, 2, 3 }
};

struct { int i; } ent3[][2] = {
	// this checks the same passing but also that we preserve the .i designator
	// { [1].i = 1, 2, 3 }
	[1][1] = { .i = 1, 2, 3 }
};
