// RUN: %ucc -fsyntax-only %s

g(int x[])
{
	return x[0];
}

f(x)
	int x[];
{
	return x[0];
}

h(x)
	__typeof(f()) x;
{
	return x;
}
