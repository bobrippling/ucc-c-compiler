// RUN: %ucc -c %s
// RUN: %ucc -S %s -o- | %asmcheck %s

e();

f()
{
	int a[][2] = { 1, 2, 3, 4 };
	e(a);
}

g()
{
	struct
	{
		int i;
		struct
		{
			int j, k;
		} a;
	} b = { 1, 2, 3 };
	e(&b);
}

h()
{
	int c[4];
	c[0] = 0;
	c[1] = 1;
	c[2] = 2;
	c[3] = 3;
	e(c);
}
