// RUN: %ucc -fsyntax-only %s

typedef struct A
{
	int A;
} A;

main()
{
	A A;

	A.A = 3;
}
