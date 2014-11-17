// RUN: %check -e %s

main()
{
	struct A
	{
		int i;
	} a, b, c;

	a = b ? : c; // CHECK: error: struct involved in ?:
}
