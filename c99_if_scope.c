struct A
{
	int i;
};

main()
{
	if(sizeof(struct A { int i, j; })) // C99 - defined _only_ for if-scope
		;

	// C99 = sizeof(int), C90 = 2 * sizeof(int)
	printf("%d\n", sizeof(struct A));
}
