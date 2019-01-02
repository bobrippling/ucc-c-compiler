// RUN: %ucc -fsyntax-only %s

main()
{
	struct A { int j; } *p;
	struct B { void *p; };
	f(p->j);

	{
		struct A { int i; } *p; // new type
		struct B b;             // use old type

		f(p->i, b.p);
	}
}

f1()
{
	typedef struct A A;
	struct A { int j; };

	{
		struct A { int i; };
		A a = { .j = 2 };

		typedef struct A A;
		A b = { .i = 3 };
	}
}

typedef struct A A;

f2()
{
	struct A { int i; };
}

struct Outside
{
	int i;
};

f3()
{
	//struct Outside;
	struct B { struct Outside/* refers to ::Outside*/ *p; int b; };
	struct Outside { struct B *p; int a; };

	struct Outside o;
	o.p; o.a;

	struct B b;
	b.p->i; b.b;
}

f4()
{
	struct Outside;
	struct B { struct Outside *p; int b; };
	struct Outside { struct B *p; int a; };

	struct Outside o;
	o.p; o.a;

	struct B b;
	b.p->p;
	b.p->p->p->a;
	b.b;
}
