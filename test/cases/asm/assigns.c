// RUN: %ucc -c %s

int gbl;

f(int p, int *q)
{
	int x;

	*(int *)5 = 3;

	*(int *)p = 1;

	*q = 5;

	x = 2;

	gbl = x;
}

g()
{
	int *p;

	p = &gbl;

	*p = 3;
}
