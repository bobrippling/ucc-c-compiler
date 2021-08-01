// RUN: %ucc -fsyntax-only %s

struct A
{
	int y, x;
};

extern struct A a[];

f()
{
	extern struct A b[];

	a[1].x = 2;
	b[1].x = 2;
}
