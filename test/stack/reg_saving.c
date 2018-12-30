// RUN: %ocheck 0 %s -DFLOAT=float
// RUN: %ocheck 0 %s -DFLOAT=int

char gc;

void g(FLOAT fp, int a, int b, int c, char *src, int line, char *fname, char *func)
{
#define CHECK(x, y) if(x != y) abort()
	CHECK(a, 0);
	CHECK(b, 3);
	CHECK(c, 5);

	CHECK(src, &gc);
	CHECK(line, 7);
	CHECK(fname, &gc + 1);
	CHECK(func, &gc + 2);
}

void f(FLOAT fp, char *src, int line, char *fname, char *func)
{
	g(fp, 0, 3, 5, src, line, fname, func);
}

main()
{
	f(0, &gc, 7, &gc + 1, &gc + 2);
	return 0;
}
