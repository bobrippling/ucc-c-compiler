// RUN: %check --prefix=range %s -Wenum-out-of-range
// RUN: %check %s

enum A
{
	X
};

f(enum A);

main()
{
	f(X); // CHECK: !/warn/
	f(1); // CHECK-range: warning: value 1 is out of range for 'enum A'
}
