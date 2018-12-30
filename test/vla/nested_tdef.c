// RUN: %ocheck 0 %s

xs, ys;

int x(){ xs++; return 3; }
int y(){ ys++; return 4; }


void abort(void);
typedef int int32_t;

tdef_btype()
{
	int32_t ar[x()];

	ar[0] = 3;
	ar[2] = 2;
	ar[1] = 1;

	if(sizeof ar != 3 * sizeof(int32_t))
		abort();
}

tdef_artype()
{
	typedef int32_t intar[x()];

	intar ar[y()];

	const int x2 = 3, y2 = 4;

	if(sizeof ar != y2 * x2 * sizeof(int32_t))
		abort();
	if(sizeof ar != y2 * sizeof *ar)
		abort();
	if(sizeof ar != y2 * x2 * sizeof **ar)
		abort();

	if(sizeof ar != 12 * sizeof(int32_t))
		abort();

	if(sizeof *ar != 3 * sizeof(int32_t))
		abort();
}

main()
{
	tdef_btype();
	if(xs != 1)
		abort();
	if(ys != 0)
		abort();

	tdef_artype();

	if(xs != 2)
		abort();
	if(ys != 1)
		abort();

	return 0;
}
