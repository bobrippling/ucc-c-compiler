// RUN: %check -e %s

main()
{
	struct A
	{
		int i;
	} a, b;

	a += b; // CHECK: /error: struct involved in compound assignment/
}
