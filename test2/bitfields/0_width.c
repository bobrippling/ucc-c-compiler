// RUN: %ocheck 0 %s

struct A
{
	int x : 3;
	int : 0; // forces y to align on boundary of next bitfield type
	int y : 4;
};

chk(int a, int b)
{
	if(a != 3 || b != 5)
		abort();
}

main()
{
	struct A a;

	a.x = 3;
	a.y = 5;

	chk(a.x, a.y);

	// should be packed like ints
	chk(0[(int *)&a], 1[(int *)&a]);

	return 0;
}
