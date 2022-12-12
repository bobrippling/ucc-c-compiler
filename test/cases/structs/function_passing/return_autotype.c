// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

typedef struct A {
	int a, b;
} A;

A f()
{
	return (A){ 1, 2 };
}

main()
{
#include "../../ocheck-init.c"
	__auto_type a = f();

	if(a.a != 1 || a.b != 2)
		abort();

	return 0;
}
