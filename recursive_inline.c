f(int);

g(int i)
{
	return 2 + f(i);
}

f(int i)
{
	return 1 + g(i);
}

main()
{
	return f(2);
}
