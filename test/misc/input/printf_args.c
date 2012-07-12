main(int argc, char **argv)
{
	for(__typeof(0) i = 0; i < argc; i++)
		printf("argv[%d] = \"%s\"\n", i, argv[i]);
	return 0;
}
