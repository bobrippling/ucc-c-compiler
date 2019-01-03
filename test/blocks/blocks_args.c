// RUN: %ocheck 5 %s

main()
{
	int (^add_2)(int) = ^(int a){
		return a + 2;
	};

	return add_2(3);
}
