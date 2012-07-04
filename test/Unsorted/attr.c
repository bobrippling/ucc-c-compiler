int f() __attribute__((unused, tim, format(printf, 5, 2), crab))
{
}

x(int i __attribute__((unused)))
{
}

int use_me() __attribute__((warn_unused))
{
}

main()
{
	x(5);
	f(5);
	use_me();
}
