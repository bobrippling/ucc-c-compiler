/*
main()
{
	int (^f)(int) = ^int (int i) {printf("hi %d\n", i); return 0;};

	^void {
		printf("test\n");
	}();

	f(5);
}

*/

int p(int(char));

int p(int x(char))
{
	x('B');
}

q(char c)
{
	printf("%c\n", c);
}

main()
{
	int (^x)(char) = ^int(char tim){printf("%c\n", tim);};
	int (^(*y))(char) = &x;
	//void (^z)(void) = 0;

	x('a');
	(*y)('a');
	(0 ? x : *y)('a');

	p(q);
}
