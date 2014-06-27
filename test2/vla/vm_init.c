// RUN: %ucc -fsyntax-only %s

f(int n)
{
	int ar[n];
	int (*p)[n] = &ar;

	g(p);
}
