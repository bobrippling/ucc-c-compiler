struct A
{
	int before;
	struct
	{
		int j;
		int i;
	};
	int after;
};

struct B
{
	int b;
	union
	{
		int a, b, c;
	};
	int a;
};

f(struct A *p)
{
	p->i = 2;
}

g(struct B *p)
{
	p->b = 1;
}

main()
{
	struct A a;
	struct B b;
	f(&a);
	g(&b);
	return a.i + b.a + b.b + b.c;
}
