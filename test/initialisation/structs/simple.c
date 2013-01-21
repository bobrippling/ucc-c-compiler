struct A
{
	int i;
	char c;
	int j;
};

struct A globl = {
	1,
	'2',
	3,
	//4
};

main()
{
	struct A stack = {
		5, '6', 7
	};

	printf("globl = { %d, %c, %d }\n",
			globl.i, globl.c, globl.j);

	printf("stack = { %d, %c, %d }\n",
			stack.i, stack.c, stack.j);
}
