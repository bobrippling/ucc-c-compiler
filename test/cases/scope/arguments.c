// RUN: %ucc -fsyntax-only %s

g(int subfunc(int subfunc1, char buf[const sizeof subfunc1]));

f(int x, char y, char buf[static (__typeof(y))sizeof x])
{
	return buf[5];
}
