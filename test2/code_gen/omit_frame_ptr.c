// RUN: %archgen %s 'x86_64,x86:!/r[sb]p/'

f()
{
	return 5;
}

g(int i)
{
	return 6;
}

char c;

v(int a, ...)
{
	return c;
}
