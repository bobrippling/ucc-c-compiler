main()
{
	int a[1 ? 5 : -1] __attribute__((unused));
	return 0 ?: 5;
}
