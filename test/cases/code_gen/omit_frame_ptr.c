// RUN: %ucc -target x86_64-linux -S -o %t %s
// RUN: ! grep '/r[sb]p/' %t

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
