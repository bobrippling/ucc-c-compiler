// RUN: %ucc -o %t %s
// RUN: %t | %output_check 2 'inline call' hi
// RUN: %t; [ $? -eq 3 ]

main()
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
