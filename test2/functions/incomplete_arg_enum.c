// RUN: %check -e %s

f(a, b, c)
	enum A a; // CHECK: error: enum A is incomplete
{
	return a + b + c;
}

main()
{
	f(1, 2, 3);
}
