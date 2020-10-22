// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

void f(char *p)
{
	static const char expected[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	for(int i = 0; i < 9; i++)
		if(expected[i] != p[i])
			abort();
}

main()
{
	f((char[]){1, 2, 3, 4, 5, 6, 7, 8, 9});

	return 0;
}
