// RUN: %check               %s -Wno-init-missing-struct-zero
// RUN: %check --prefix=zero %s -Winit-missing-struct-zero

main()
{
	typedef struct A
	{
		int i, j, k;
		char buf[3];
	} A;

	A a = {};

	A b = { 0 }; // CHECK: !/warn/
	             // CHECK-zero: ^ warning: 3 missing initialisers for 'struct A'

	A c = { 1 }; // CHECK: warning: 3 missing initialisers for 'struct A'
	             // CHECK-zero: ^ warning: 3 missing initialisers for 'struct A'
}
