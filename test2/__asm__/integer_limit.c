// RUN: %check -e %s

#define LIMIT(c, n) __asm("" : : c(n))

main()
{
	LIMIT("I", 5); // CHECK: !/warn/
	LIMIT("I", 31); // CHECK: !/warn/
	LIMIT("I", -1); // CHECK: error: input constraint unsatisfiable
	LIMIT("I", 32); // CHECK: error: input constraint unsatisfiable

	LIMIT("J", 5); // CHECK: !/warn/
	LIMIT("J", 63); // CHECK: !/warn/
	LIMIT("J", -1); // CHECK: error: input constraint unsatisfiable
	LIMIT("J", 64); // CHECK: error: input constraint unsatisfiable

	LIMIT("K", 5); // CHECK: !/warn/
	LIMIT("K", -3); // CHECK: !/warn/
	LIMIT("K", 129); // CHECK: error: input constraint unsatisfiable
	LIMIT("K", -129); // CHECK: error: input constraint unsatisfiable

	LIMIT("M", -1); // CHECK: error: input constraint unsatisfiable
	LIMIT("M", 0); // CHECK: !/warn/
	LIMIT("M", 1); // CHECK: !/warn/
	LIMIT("M", 2); // CHECK: !/warn/
	LIMIT("M", 3); // CHECK: !/warn/
	LIMIT("M", 4); // CHECK: error: input constraint unsatisfiable

	LIMIT("N", 0); // CHECK: !/warn/
	LIMIT("N", 256); // CHECK: !/warn/
	LIMIT("N", -1); // CHECK: error: input constraint unsatisfiable
	LIMIT("N", 257); // CHECK: error: input constraint unsatisfiable
}
