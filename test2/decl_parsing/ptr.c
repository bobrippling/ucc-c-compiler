int i;

int *x()
{
	return &i;
}

main()
{
	*x() = 5;

	return i;
}
