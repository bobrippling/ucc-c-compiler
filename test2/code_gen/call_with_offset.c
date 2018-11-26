// RUN: %ocheck 0 %s

typedef struct
{
	int x, y;
} pt;

set(int x, int y, int *a, int *b)
{
	*a = 5;
	*b = 10;
}

f(pt *pt)
{
	// ensure this generates the right addresses.
	// need a few bonus arguments to make sure register
	// transfers are needed
	set(1, 2, &pt->x, &pt->y);
}

main()
{
	pt xy = { 0 };

	f(&xy);

	_Noreturn void abort(void);

	if(xy.x != 5)
		abort();

	if(xy.y != 10)
		abort();

	return 0;
}
