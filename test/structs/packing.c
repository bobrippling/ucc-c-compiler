f(struct { int a, b, c; } *p)
{
	return p->c;
}

main()
{
	struct
	{
		int a, b, c;
	} a;

	a.a = 1;
	a.b = 2;
	a.c = 3;

	return f(&a) == 3 ? 0 : 1;
}
