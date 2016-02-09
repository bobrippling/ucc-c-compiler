// test emission of locally scoped global objects - functions and externs

f(int i)
{
	{
		int i = 3;
		{
			extern i; // locally scoped
			i = g();
		}
	}

	return i;
}
