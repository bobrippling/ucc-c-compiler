// RUN: %ocheck 0 %s

int called;

int p1()
{
	called = 1;
}

main()
{
	struct
	{
		const char *n;
		void (*f)(void);
	} f;

	f.n = "hi";
	f.f = p1;

	f.f();

	return called ? 0 : 1;
}
