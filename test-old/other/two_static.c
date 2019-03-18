f()
{
	int j = 0;

	{
		static int i;
		i++;
		j += i;
	}
	{
		static int i;
		i++;
		j += i;
	}

	return j;
}
