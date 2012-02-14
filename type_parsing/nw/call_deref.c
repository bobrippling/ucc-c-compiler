printf();

x()
{
	printf("x()\n");
}

main()
{
	int (*p)();
	p = x;

	(*p)();
	((int (*)())NULL)();
	// TODO
}
