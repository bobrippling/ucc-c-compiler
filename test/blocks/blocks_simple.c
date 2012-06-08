f()
{
	int (^b)();
	int *(^q)() = ^{ static int i = 2; return &i; };

	printf("%d\n", *q());

	b = ^{
		printf("hi\n");
		return 3;
	};

	^{printf("inline call\n");}();

	return b();
}

main()
{
	return f();
}
