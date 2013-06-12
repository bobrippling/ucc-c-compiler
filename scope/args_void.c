any2();

any2()
{
	return 0;
}

main()
{
	any2(2); // CHECK: /warning: too many arguments to implicitly void/
}
