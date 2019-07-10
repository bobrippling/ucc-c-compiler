// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));



main()
{
	struct { int i, j, k; } a[][2] = {
		{ { 1, 2, 3 }, { 4, 5, 6 } },
		{ { 7, 8, 9 }, { 10, 11, 12 } },
	};

#define CHECK(p, x, y, z) \
	if(p.i != x || p.j != y || p.k != z) \
		abort()

	CHECK(a[0][0], 1, 2, 3);
	CHECK(a[0][1], 4, 5, 6);
	CHECK(a[1][0], 7, 8, 9);
	CHECK(a[1][1], 10, 11, 12);

	return 0;
}
