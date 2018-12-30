// RUN: %ocheck 0 %s

main()
{
	__label__ x;
	void *outer_x = &&x;

	int i = 0;
	{
		x:
		if(i++)
			return 0;
	}

	{
		__label__ x;
		goto x;
		{
			x:
			goto *outer_x;
		}
	}

	{
		__label__ y;
		goto y;
		{
			y:
			goto x;
		}
	}
}
