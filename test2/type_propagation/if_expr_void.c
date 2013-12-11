// RUN: %check -e %s

main()
{
	__typeof__(*(0 ? (int*)0 : (void*)1)) x; // CHECK: !/warn|error/

	f(*x); // CHECK: error: invalid indirection applied to typeof(expr: dereference) (aka 'void')
}
