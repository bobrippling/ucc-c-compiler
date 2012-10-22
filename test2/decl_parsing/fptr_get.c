int x()
{
	printf("yo\n");
}

int (*f(void))()
{
	return x;
}

main()
{
	f()();
	return 0;
}
