// RUN: %layout_check %s
struct
{
	int x, y; struct { int x, y, z; } a;
} ent1[3] = {
	[0].x=1, [0].y=2,

	{ .x=3, .y=4 },

	5, [2].y=6
};
