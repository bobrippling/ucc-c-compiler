main()
{
	int a;
	{
		int b;
		{
			int c, b; // shares stack space with d & a
		}
		{
			int d, a;
		}
	}
}

