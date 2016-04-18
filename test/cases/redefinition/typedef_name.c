// RUN: %check -e %s

typedef struct A
{
	int A;
} A;

A A; // CHECK: error: mismatching definitions of "A"

main()
{
	A.A = 3;
}
