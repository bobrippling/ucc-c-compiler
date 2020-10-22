// RUN: %ocheck 0 %s

abort();
#define assert(x) if(!(x)) abort()

f()
{
	int a[][2] = { 1, 2, 3, 4 };
	assert(a[0][0] == 1);
	assert(a[0][1] == 2);
	assert(a[1][0] == 3);
	assert(a[1][1] == 4);
}

g()
{
	struct
	{
		int i;
		struct
		{
			int j, k;
		} a;
	} b = { 1, 2, 3 };
	assert(b.i == 1);
	assert(b.a.j == 2);
	assert(b.a.k == 3);
}

h()
{
	int c[4];
	c[0] = 0;
	c[1] = 1;
	c[2] = 2;
	c[3] = 3;
	for(int i = 0; i < 4; i++)
		assert(c[i] == i);
}

main()
{
	f();
	g();
	h();
	return 0;
}
