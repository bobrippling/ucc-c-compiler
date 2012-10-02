main()
{
	char (^f)() = ^{
		return 'a';
	};

	return f();
}
