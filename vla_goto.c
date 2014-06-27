main()
{
	int n = 5;
	goto a;
	{
		int (*x)[n];
a:;
	}

	goto b;
	{
		int x[n];
b:;
	}
}
