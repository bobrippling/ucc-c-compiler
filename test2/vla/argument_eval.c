// RUN: %ocheck 0 %s

extern void abort(void);

as, bs, fs;

static int a(){ as++; return 2; }
static int b(){ bs++; return 2; }

static int f(int p[a()][b()])
{
	fs++;
	return p[0][0] + p[0][1] + p[1][0] + p[1][1];
}

static void assert(_Bool b)
{
	if(!b)
		abort();
}

int main()
{
	int ar[a()][b()];

	assert(as == 1);
	assert(bs == 1);
	assert(fs == 0);

	ar[0][0] = 5;
	ar[0][1] = 4;
	ar[1][0] = 3;
	ar[1][1] = 2;

	assert(as == 1);
	assert(bs == 1);
	assert(fs == 0);

	assert(f(ar) == 14);

	assert(as == 2);
	assert(bs == 2);
	assert(fs == 1);

	return 0;
}
