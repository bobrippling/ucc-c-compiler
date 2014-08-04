// RUN: %ucc -g %s -o %t

f()
{
	return 5;
}

g(int i)
{
	return 6;
}

h(int i)
{
	return i + 2;
}

char c;

v(int a, ...)
{
	return c;
}

main()
{
	if(f() != 5)
		abort();
	if(g(0) != 6)
		abort();
	if(h(1) != 3)
		abort();

	c = 3;
	if(v(1, 2, 3, 4) != 3)
		abort();

	return 0;
}
