int f(int x[static /*const*/ 10])
{
	return *++x;
}

int g(int x[10]);

main()
{
	int x[5];
	x[1] = 2;
	f(x);

	g(x); // no warn

	//int y[1];
	//int *y;
	//#define y (void *)0
	//pipe(y);
}
