// RUN: %ocheck 3 %s

main()
{
	int (^b)(void) = ^{
		return 3;
	};
	return b();
}
