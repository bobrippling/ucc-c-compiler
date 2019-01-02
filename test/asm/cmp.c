// RUN: %ucc %s -c

a(int a, int b)
{
	int i;

	i = (5 > 2) + (6 < 3);

	i = 1 < (2 + 3) < 4;
}

f(int a)
{
	if(a > 2)
		q();
}
