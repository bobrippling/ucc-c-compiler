// RUN: %ocheck 0 %s

typedef struct
{
	int x[1];
} A;

A *f()
{
	static A a = { 1 };
	return &a;
}

main()
{
	f()->x[0] = 3;

	if(f()->x[0] != 3)
		abort();

	return 0;
}
