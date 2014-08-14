// RUN: %check -e %s -fshow-inlined

#define ai __attribute((always_inline))

ai f(int);

ai g(int i)
{
	// this should fail to inline eventually
	return 2 + f(i); // CHECK: note: function inlined
// CHECK: ^error: couldn't always_inline call: recursion too deep
}

ai f(int i)
{
	return 1 + g(i); // CHECK: note: function inlined
// CHECK: ^error: couldn't always_inline call: recursion too deep
}

main()
{
	return f(2); // CHECK: note: function inlined
}
