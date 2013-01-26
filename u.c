struct A
{
	int i;
	struct
	{
		int b;
	};
};

f(struct A *p)
{
	p->b = 2;
}

main()
{
	struct A a;
	f(&a);
	return a.b;
}
