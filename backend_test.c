struct A
{
	int a, b;
};

f()
{
	struct A p;
	p.a = 1;
	p.b = 2;
}

ptr(struct A *p)
{
	p->a = 1;
	p->b = 1;
}

main()
{
	int i = 5;
	f(&i);

	f(1, 2);

	return i;
}

yo(unsigned long n, int ty_sz)
{
	return (sizeof(long) - ty_sz) * 8;
}
