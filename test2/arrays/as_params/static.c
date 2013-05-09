// RUN: %ucc -o %t %s
// RUN: %check %s

int f(int x[static /*const*/ 10])
{
	return *++x;
}

int g(int x[10]){}

main()
{
	int x[5];
	x[1] = 2;
	if(f(x) != 2) // CHECK: /warning: array of size 5 passed where size 10 needed/
		abort();

	g(x); // no warn

	int y[1];
	int pipe(int [static 2]);
	pipe((void *)0); // CHECK: /warning: passing null-pointer where array expected/
	pipe(y); // CHECK: /warning: array of size 1 passed where size 2 needed/
}
