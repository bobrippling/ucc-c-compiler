// 1, 2, 3, 4, 5, 6
struct S2
{
	int x, y;
} a4[3] = {
	// FIXME: some are lost
	// a4
	[0].x=1, [0].y=2,
	{
		// a4[1]
		.x=3, .y=4
	},
	// a4[2]
	5, [2].y=6
};
