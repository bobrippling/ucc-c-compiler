// RUN: %ucc -fsyntax-only %s

struct A
{
	int i;
};

void f(void)
{
	struct A a;

	struct A
	{
		char j;
	} x;

	a.i = 3;
	x.j = 2;
}
