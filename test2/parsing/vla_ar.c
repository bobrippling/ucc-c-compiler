// RUN: echo TODO; false
int f(int x[*]);
int g(int i, int x[i]);
int h(int x[i], int i); // error

main()
{
	int x[2];
	g(5, x); // clang doesn't pick this up
}
