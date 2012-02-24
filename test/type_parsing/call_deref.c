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
	return 0;
}
