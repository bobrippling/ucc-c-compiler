// RUN: %ucc -o %t %s
// RUN: %t | %stdoutcheck %s
// RUN: %t; [ $? -eq 3 ]

// STDOUT: 2
// STDOUT-NEXT: inline call
// STDOUT-NEXT: hi

int printf(const char *, ...);

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
