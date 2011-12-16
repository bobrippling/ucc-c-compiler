main(int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; i++)
		printf("%d\n", atoi(argv[i]));

	return 0;
}
