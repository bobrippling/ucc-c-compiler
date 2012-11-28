main()
{
	int (^b)(void) = ^{
		return 3;
	};
	return b();
}
