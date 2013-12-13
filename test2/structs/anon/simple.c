// RUN: %ocheck 0 %s
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
	int p;
	union
	{
		int a, b, c;
	};
	int t;
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
	if(b.a != b.b || b.b != b.c)
		return 1;
	return a.i + b.a + b.b + b.c == 5 ? 0 : 1;
}
