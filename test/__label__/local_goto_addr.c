// RUN: %ocheck 6 %s

main()
{
	__label__ z;
	void *y = &&z;
	void *p;
	int i = 0;

	{
		__label__ z;
		z:
		p = &&z;
		if(i++ == 5)
			goto *y;
	}

	goto *p;
z:
	return i;
}
