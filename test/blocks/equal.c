// RUN: %ocheck 97 %s

main()
{
	char (^f)() = ^{
		return 'a';
	};

	return f();
}
