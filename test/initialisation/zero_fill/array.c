// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
	int x[4] = { 1 };

	if(x[0] != 1
	|| x[1]
	|| x[2]
	|| x[3])
	{
		abort();
	}

	return 0;
}
