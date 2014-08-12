// RUN: %inline_check %s
// RUN: %ocheck 20 %s

f(int i)
{
	return 3 + i;
}

g(int i)
{
	return 2 * f(i);
}

main()
{
	return g(7);
}
