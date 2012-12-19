f(int *) __attribute__((nonnull));

main()
{
	int i;

	f((void *)0);
	f(&i);
	f(0);
}
