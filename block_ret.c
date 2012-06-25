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

int p(int(int));

main()
{
	int (^x)(char);
	void (^z)(void);
	int (^(*y))(char) = &x;

	x('a');
	(*y)('a');
	(1 ? x : *y)('a');
}
