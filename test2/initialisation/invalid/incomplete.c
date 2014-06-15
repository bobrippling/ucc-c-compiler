// RUN: %check -e %s

typedef struct A A;

main()
{
	extern A x[]; // CHECK: error: array has incomplete type 'A (aka 'struct A')'

	//f(x[1]);
}
