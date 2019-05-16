// RUN: %ocheck 4 %s -std=c99
// RUN: %ocheck 8 %s -std=c89

struct A
{
	int i;
};

main()
{
	if(sizeof(struct A { int i, j; })) // C99 - defined _only_ for if-scope
		;

	/*
	 * C90 = 8
	 * C99 = 4
	 */
	return sizeof(struct A);
}
