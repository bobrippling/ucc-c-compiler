// RUN: %check -e %s

f(int); // CHECK: note: 'f' declared here

main()
{
	int (*p)(int) = f; // CHECK: note: 'p' declared here

	f(1, 2); // CHECK: error: too many arguments to function f (got 2, need 1)
	p(1, 2); // CHECK: error: too many arguments to function p (got 2, need 1)
	(*p)(1, 2); // CHECK: error: too many arguments to function (got 2, need 1)
}
