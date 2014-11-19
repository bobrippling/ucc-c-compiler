main()
{
	int a, b, c;

	a = 5, b = 2;

	asm(
			"cmpl %2, %1;"
			"setg %0;"
			: "=m" (c)
			: "m" (a), "r" (b)
		 );

	return c;
}
