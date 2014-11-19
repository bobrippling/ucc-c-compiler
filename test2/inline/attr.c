// RUN: %check %s -fshow-inlined

__attribute((always_inline))
void f()
{
}

__attribute((noinline))
int g()
{
	f(); // CHECK: note: function inlined
	return 5;
}

main()
{
	return g(); // CHECK: !/function inlined/
}
