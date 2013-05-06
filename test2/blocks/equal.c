// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 97 ]
main()
{
	char (^f)() = ^{
		return 'a';
	};

	return f();
}
