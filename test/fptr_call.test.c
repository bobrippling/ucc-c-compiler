printf();

f()
{
	printf("f()\n");
}

(*p(int i))()
{
	printf("p(%d)\n", i);
	return f;
}

main()
{
	int (*ptr(int))();
	int (*r)();
	ptr = p;
	p(5);
	r = ptr(1);
	r();
}
