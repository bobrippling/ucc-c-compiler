printf();

x()
{
	printf("x()\n");
}

main()
{
	int (*p)();
	p = x;

	//*p;

	(*p)();
	//((int (*)())0)();
}
