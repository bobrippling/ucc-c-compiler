// RUN: %ucc -fsyntax-only %s

int f(int x[*]);
int g(int i, int x[i]);

main()
{
	int x[2];
	f(x);
	g(5, x); // size mismatch not noticed
}
