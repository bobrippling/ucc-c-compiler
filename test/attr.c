int f() __attribute__((__unused__, tim, __format__(__printf__, 5, 2), crab))
{
}

x(int i __attribute__((__unused__)))
{
}

int use_me() __attribute__((__warn_unused__))
{
}

main()
{
	x(5);
	f(5);
	use_me();
}
