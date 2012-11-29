// RUN: echo TODO %s
// RUN: false

typedef struct
{
	int n1, n2, n3;
} triplet;

triplet nlist[2][3] =
{
	{ {  1, 2, 3 }, {  4, 5, 6 }, {  7, 8, 9 } },  /* Row 1 */
	{ { 10,11,12 }, { 13,14,15 }, { 16,17,18 } }   /* Row 2 */
};



triplet nlist2[2][3] =
{
	{  1, 2, 3 },{  4, 5, 6 },{  7, 8, 9 },   /* Line 1 */
	{ 10,11,12 },{ 13,14,15 },{ 16,17,18 }    /* Line 2 */
};



struct list
{
	int i, j, k;
	float m[2][3];
} x = {
	1,
	2,
	3,
	{4.0, 4.0, 4.0}
};


union
{
	char x[2][3];
	int i, j, k;
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
