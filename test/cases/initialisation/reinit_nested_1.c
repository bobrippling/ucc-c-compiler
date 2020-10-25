// RUN: %layout_check %s
struct S2
{
	int x, y;
} ent1[3] = {
	[0].x=1, [0].y=2,

	{ .x=3, .y=4 },

	5, [2].y=6
};
