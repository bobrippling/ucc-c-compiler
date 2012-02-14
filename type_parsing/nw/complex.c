printf();

x()
{
	printf("yay\n");
}

int (*getptr())()
{
	return x;
}

main()
{
	int (*p)() = getptr();

	p();
	getptr()();

	(*p)();
	(*getptr())();
}
