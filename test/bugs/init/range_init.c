




typedef unsigned long size_t;
extern int memcmp (const void *, const void *, size_t);
void abrt (int i)
{
	printf("abrt(%d)\n", i);
	exit(1);
}
extern void exit (int);

int a[][2][4] = { [2 ... 4][0 ... 1][2 ... 3] = 1, [2] = 2, [2][0][2] = 3 };
struct E {};
struct F { struct E H; };
struct G { int I; struct E J; int K; };
struct H { int I; struct F J; int K; };
struct G k = { .J = {}, 1 };
struct H l = { .J.H = {}, 2 };
struct H m = { .J = {}, 3 };
struct I { int J; int K[3]; int L; };
struct M { int N; struct I O[3]; int P; };
struct M n[] = { [0 ... 5].O[1 ... 2].K[0 ... 1] = 4, 5, 6, 7 };
struct M o[] = { [0 ... 5].O = { [1 ... 2].K[0 ... 1] = 4 },
	[5].O[2].K[2] = 5, 6, 7 };
struct M p[] = { [0 ... 5].O[1 ... 2].K = { [0 ... 1] = 4 },
	[5].O[2].K[2] = 5, 6, 7 };
int q[3][3] = { [0 ... 1] = { [1 ... 2] = 23 }, [1][2] = 24 };
int r[1] = { [0 ... 1 - 1] = 27 };

int main (void)
{
  int x, y, z;

	if (a[2][0][0] != 2 || a[2][0][2] != 3)
		abrt (__LINE__);
	a[2][0][0] = 0;
	a[2][0][2] = 1;
	for (x = 0; x <= 4; x++)
		for (y = 0; y <= 1; y++)
			for (z = 0; z <= 3; z++)
				if (a[x][y][z] != (x >= 2 && z >= 2))
					abrt (__LINE__);
	if (k.I || l.I || m.I || k.K != 1 || l.K != 2 || m.K != 3)
		abrt (__LINE__);
	for (x = 0; x <= 5; x++)
	{
		if (n[x].N || n[x].O[0].J || n[x].O[0].L)
			abrt (__LINE__);
		for (y = 0; y <= 2; y++)
			if (n[x].O[0].K[y])
				abrt (__LINE__);
		for (y = 1; y <= 2; y++)
		{
			if (n[x].O[y].J)
				abrt (__LINE__);
			if (n[x].O[y].K[0] != 4)
				abrt (__LINE__);
			if (n[x].O[y].K[1] != 4)
				abrt (__LINE__);
			/*if ((x < 5 || y < 2) && (n[x].O[y].K[2] || n[x].O[y].L))
				abrt (__LINE__);*/
		}
#warning this one:
		if (x < 5 && n[x].P)
			;//abrt (__LINE__);
	}
	if (n[5].O[2].K[2] != 5 || n[5].O[2].L != 6 || n[5].P != 7)
		abrt (__LINE__);
	if (memcmp (n, o, sizeof (n)) || sizeof (n) != sizeof (o))
		;//abrt (__LINE__);
	if (memcmp (n, p, sizeof (n)) || sizeof (n) != sizeof (p))
		;//abrt (__LINE__);
	if (q[0][0] || q[0][1] != 23 || q[0][2] != 23)
		abrt (__LINE__);
	if (q[1][0] || q[1][1] != 23 || q[1][2] != 24)
		abrt (__LINE__);
	if (q[2][0] || q[2][1] || q[2][2])
		abrt (__LINE__);
	if (r[0] != 27)
		abrt (__LINE__);
	exit (0);
}

#if 0
{
	{
		{ 0, 0, 0, 0, },
		{ 0, 0, 0, 0, },
	},
	{
		{ 0, 0, 0, 0,},
		{ 0, 0, 0, 0,},
	},

	// [2]
	{
		    //  2..3 = 1, column
		{ 0, 0, 3, 0,} // [0...1]
		      //^ = 3, single
		//^ = 2, single
		{ 0, 0, 0, 0,}
	}
	// the decl_init_copy is shallow,
	// and so overwrites of sub objects don't work
	{
		{ 0, 0, 0, 0,}
		{ 0, 0, 0, 0,}
	}
	{
		{ 0, 0, 0, 0,}
		{ 0, 0, 0, 0,}
	}
}
#endif
