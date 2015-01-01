extern _Noreturn void abort(void);

int g(int a, int b)
{
	if(b)
		abort();
	if(a != 5)
		abort();

	return 72;
}

dup(int x)
{
	return x + 1;
}

f(int a, int b)
{
	return g(a, b ? dup(b) : 0);
}

main()
{
	f(5, 0);
}
