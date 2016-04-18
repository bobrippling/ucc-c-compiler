// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
	int a[][2] = {
		{ 1, 2,},
		{ 3, 4 },
	};

	if(a[0][0] != 1
	|| a[0][1] != 2
	|| a[1][0] != 3
	|| a[1][1] != 4)
	{
		abort();
	}

	return 0;
}
