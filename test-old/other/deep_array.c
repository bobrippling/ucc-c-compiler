int printf();
int strlen();

int f(int argc, char **argv)
{
	int i;

	if(argc != 2 || strlen(argv[1]) != 3){
		printf("need %s xyz\n", argv[0]);
		return 1;
	}

	for(i = 0; i < 3; i++)
		printf("argv[1][%d] = %c (%d)\n", i, argv[1][i], argv[1][i]);

	return 0;
}

main()
{
	const char *ps[3];

	ps[0] = "hi";
	ps[1] = "there";
	ps[2] = 0;

	f(2, ps);
}
