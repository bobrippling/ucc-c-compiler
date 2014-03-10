clean(int *p)
{
	printf("hi\n");
}

main()
{
	int i __attribute((cleanup(clean)));
	int x[2] __attribute((cleanup(clean)));

	return 3;
}
