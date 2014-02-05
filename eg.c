main()
{
	int *p = (typeof(p))0;

	for(int _Alignas(long) x = 0; x; x++);
}

typedef int f(void)
{
	return 3;
}

typedef char c = 3;
