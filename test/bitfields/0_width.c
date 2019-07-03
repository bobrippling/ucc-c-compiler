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

	memset(&a, 0, sizeof a);

	a.x = 3;
	a.y = 5;

	chk(a.x, a.y);

	/* should be packed like ints
	 * note that this relies on the memset() above,
	 * as bitfields don't take part in initialisation,
	 * even = { 0 }.
	 */
	chk(0[(int *)&a], 1[(int *)&a]);

	return 0;
}
