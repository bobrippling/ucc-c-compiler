// RUN: %ucc -o %t %s
// RUN: %t | %output_check a a a B

int dispatch(int(char));

int dispatch(int x(char))
{
	x('B');
}

pc(char c)
{
	printf("%c\n", c);
}

main()
{
	int (^x)(char) = ^int(char tim){printf("%c\n", tim);};
	int (^(*y))(char) = &x;
	void (^z)(void) = 0;

	x('a');
	(*y)('a');
	(0 ? x : *y)('a');

	dispatch(pc);
}
