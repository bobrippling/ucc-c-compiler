// RUN: %layout_check %s
// RUN: %check %s

typedef struct
{
	int n1, n2, n3;
} triplet;

triplet nlist[2][3] =
{
	{ {  1, 2, 3 }, {  4, 5, 6 }, {  7, 8, 9 } },
	{ { 10,11,12 }, { 13,14,15 }, { 16,17,18 } }
};



triplet nlist2[2][3] =
{
	{  1, 2, 3 /* ..., ... */ }, // CHECK: /warning: missing braces/
	{  4, 5, 6 }, // CHECK: /warning: missing braces/
	{  7, 8, 9 }, // CHECK: /warning: excess/
	{ 10,11,12 },{ 13,14,15 },{ 16,17,18 }
};



struct list
{
	int i, j, k;
#ifdef FLOATS
	float m[2][3];
#endif
} x = {
	1,
	2,
	3,
#ifdef FLOATS
	{4.0, 4.0, 4.0}
#endif
};


union
{
	char x[2][3];
	int i, j, k; // _separate_ unions ents
} y = {
	/*char[2][3]*/{
		/*char[3]*/{'1'},
		/*char[3]*/{'4'}
	}
};

/* y = {
 * { {'1',0,0}, {'4',0,0} },
 * 0, 0, 0
 * }
 */
