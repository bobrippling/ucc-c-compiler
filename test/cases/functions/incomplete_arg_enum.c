// RUN: %check -e %s

f(a, b, c)
	enum A a; // CHECK: error: function argument "a" has incomplete type 'enum A'
{
}

main()
{
	f(1, 2, 3);
}
