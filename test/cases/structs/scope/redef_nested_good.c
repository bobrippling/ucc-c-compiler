// RUN: %ucc -fsyntax-only %s

struct B
{
	int i;
};

struct Global
{
	char *s;
};

f()
{
	struct A
	{
		int i;
		struct Global g;
	} a;

	struct B
	{
		int j, k;
	};

	a.g.s = "hi";

	struct B;

	struct B b;
	b.k = 3;

	{
		struct B x = { .k = 5 };
	}
}

struct R
{
	struct R *next;
};

struct R r = { &r };
