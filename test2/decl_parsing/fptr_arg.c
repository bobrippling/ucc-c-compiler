// RUN: %ocheck 5 %s

call_for_me(int (*f)(int), int arg)
{
	return f(arg);
}

inc(int i)
{
	return i + 1;
}

main()
{
	int (*f)(int) = inc;
	return call_for_me(f, 5);
}
