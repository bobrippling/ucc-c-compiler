#include "pack.h"

main()
{
	struct A a;

	f(&a);

	return
		a.a +
		a.b +
		a.c +
		a.l +
		a.s1 +
		a.s2 +
		a.s3 +
		a.s4 +
		a.s5 +
		a.i2;
}
