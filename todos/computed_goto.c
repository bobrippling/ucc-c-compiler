main()
{
	void *p;

	p = &&l;

	goto *p;

	return 0;
l:
	printf("hi\n");
	return 1;
}
