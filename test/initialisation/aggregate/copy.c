// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
	struct { int i, j; } x[] = {
		[1 ... 5] = { 1, 2 } // a memcpy is performed for these structs
	};

	if(x[0].i || x[0].j)
		abort();

	for(int i = 1; i < 6; i++)
		if(x[i].i != 1 || x[i].j != 2)
			abort();

	return 0;
}
