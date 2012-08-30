extern char (*(*x)[])();

main()
{
	char c;

	c = (*x[0])();
}
