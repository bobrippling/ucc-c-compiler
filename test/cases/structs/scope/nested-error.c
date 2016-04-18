// RUN: %check -e %s

typedef struct A A;

void f()
{
	struct A { int i; }; // CHECK: !/error/

	A b; // CHECK: /error/
}
