main()
{
	const __typeof("hi") str = "yo";
	typedef __typeof(1) volatile *pone;
	int i;
	pone p = &i;
	printf("%s\n", str);
	*p = 2;
	assert(i == 2);
}
