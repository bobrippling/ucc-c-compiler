main()
{
	int a = 5, b = 2, c;

	asm(
			"cmpl %2, %1;"
			"setg %0;"
			: "=m" (c)
			: "m" (a), "r" (b)
		 );

	return c;
}
