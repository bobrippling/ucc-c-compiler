struct A
{
	int i, j;
} ent1[] = {
	{ 4, 5 },
	{ 6, 7 },
	{ 8, 9 },

	[0] = { 1 }, // gives { 1, 0 } for full sub-obj override
	[1] =   2,   // gives { 2, <prev=7> }
};

// 1, 0, 2, 7, 8, 9
