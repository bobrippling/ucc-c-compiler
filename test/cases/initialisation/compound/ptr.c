// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f()
{
	abort();
}

main()
{
	int (*p)[2] = (int[][2]){ 1, 2, { 3, 4, f() } };

	if(p[0][0] != 1
	|| p[0][1] != 2
	|| p[1][0] != 3
	|| p[1][1] != 4)
	{
		abort();
	}

	return 0;
}
