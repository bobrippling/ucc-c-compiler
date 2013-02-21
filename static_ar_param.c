int f(int x[static volatile /*const*/ 10])
{
	return *++x;
}

main()
{
	int x[5];
	x[1] = 2;
	f(x);
	//int y[1];
	//int *y;
	//#define y (void *)0
	//pipe(y);
}
