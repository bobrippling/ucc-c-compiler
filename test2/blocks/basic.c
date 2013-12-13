// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 3 ]
main()
{
	int (^b)(void) = ^{
		return 3;
	};
	return b();
}
