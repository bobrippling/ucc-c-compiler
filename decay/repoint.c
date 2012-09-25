main()
{
	int x[5];
	int (*px)[5] = &x;
	int p;

	//x = &p;
	*px = &p;
}
