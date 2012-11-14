f(i);

f(i)
	int i;
{
	return i;
}

g(i, j)
	int *j;
{
	return *j + i;
}

main()
{
	return f(5);
}
