printf();

#ifndef STATIC
i = 0;
#endif

x()
{
#ifdef STATIC
	static int i = 0;
#warning using static
#endif
	printf("%d\n", ++i);
}

int (*getptr())()
{
	return x;
}

int (*getptr_addr())()
{
	return &x;
}

main()
{
	int (*p)() = getptr(), (*q)();

	q = getptr_addr();

	p();
	q();
	getptr()();
	getptr_addr()();

	(*p)();
	(*q)();
	(*getptr())();
	(*getptr_addr())();
}
