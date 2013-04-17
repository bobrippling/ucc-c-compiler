// RUN: %ocheck 0 %s

struct A
{
	int b;
	struct
	{
		int pad1;
		struct
		{
			int pad2;
			struct
			{
				int pad3;
				int i, j;
			};
		};
	};
	int k;
};

f(struct A *p)
{
	p->i = 1;
}

main()
{
	struct A a;
	f(&a);
	if(a.i != *((int *)&a + 4))
		return 1;
	return a.i == 1 ? 0 : 1;
}
