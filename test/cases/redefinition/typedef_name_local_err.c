// RUN: %check -e %s

main()
{
	typedef struct A
	{
		int A;
	} A;

	A A; // CHECK: error: mismatching definitions of "A"

	A.A = 3;
}
