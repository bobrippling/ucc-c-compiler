// RUN: %check -e %s

const int f(void);

__typeof(1 + f()) g; // constness shouldn't be propagated

main()
{
	__typeof((const int)50) x; // casts don't have qualifiers -> x is of type 'int'

	const int ki = 3;
	__typeof(ki) ki2;
	__typeof(*&ki) ki3;

	g = 2; // CHECK: !/error/
	x = 3; // CHECK: !/error/

	ki2 = 5; // CHECK: error: can't modify const expression identifier
	ki3 = 5; // CHECK: error: can't modify const expression identifier
}
