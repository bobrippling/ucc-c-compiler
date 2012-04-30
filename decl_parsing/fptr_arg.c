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
	return call_for_me(inc, 5);
}
