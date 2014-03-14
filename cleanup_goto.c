cleanup(void *p)
{
	printf("cleanup %p\n", (void *)p);
}
#define cleanup __attribute((cleanup(cleanup)))

main()
{
	cleanup int i = 0;
	goto after;

	cleanup int j = 1;
after:
	return 0;
}
