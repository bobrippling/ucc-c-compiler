// RUN: %ocheck 0 %s

enum
{
	constant = 5
};

extern void abort();

main()
{
	int expr = constant;

	// there was a problem where if we had a constant
	// first and inverted the comparison, this would change
	// != to ==, even though !=/== are commutative.
	if(constant != expr)
		abort();
}
