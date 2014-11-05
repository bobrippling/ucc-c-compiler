// RUN: %check %s -fshow-inlined -fno-inline-functions

__attribute((always_inline))
int f()
{
	return 3;
}

main()
{
	return f(); // CHECK: note: function inlined
}
