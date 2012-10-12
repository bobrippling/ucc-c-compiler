// RUN: %ucc -c %s

g()
{
	int *p;
	*p = 5;
}

f()
{
	struct A
	{
		int i;
	} a;

	a.i = 5;

	return a.i;// = 0;
}

q()
{
	int i;
	i = 3;
}
