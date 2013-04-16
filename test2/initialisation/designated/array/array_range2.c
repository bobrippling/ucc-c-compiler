f(){return 7;}
q(){return 8;}

main()
{
	char x[20] = {
		[4] = q(),
		[5 ... 9] = f(),
	};

	struct A { int i, j; } y[] = {
		[3 ... 4] = { 1, 2 }
	};

	for(int i = 0; i < 20; i++)
		printf("x[%d] = %d\n", i, x[i]);

	for(int i = 0; i < 5; i++)
		printf("y[%d] = { %d, %d }\n", i, y[i].i, y[i].j);
}
