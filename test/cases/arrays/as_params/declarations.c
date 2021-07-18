// RUN: %ucc -fsyntax-only %s

f(int [][2]);

h(int [][0]);

fine(int [static]);
fine(int [static 2]);

main()
{
	int x[3];
	int y[3];
}
