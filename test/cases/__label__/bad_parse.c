// RUN: %check -e %s

main()
{
	int i = 5;
	__label__ x; // CHECK: error: expression expected, got __label__
	x:
	return i;
}
