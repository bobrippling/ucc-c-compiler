// RUN: %ocheck 6 %s
// RUN: %ocheck 6 %s -fstack-protector-all

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
