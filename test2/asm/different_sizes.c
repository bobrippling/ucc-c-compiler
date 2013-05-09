// RUN: %ucc -c %s

long f(int i, long l)
{
	return i + l;
}

short g(short a, char b)
{
	return a + b;
}

char h(long l)
{
	return l + 1;
}

long a(char c)
{
	return c - 1;
}

short q(short a, char b)
{
	return a + b;
}
