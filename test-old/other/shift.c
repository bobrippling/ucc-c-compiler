main()
{
	int i = 1;
	for(; i < 1025; i <<= 1)
		printf("i = %d\n", i);
	return 0;
}
