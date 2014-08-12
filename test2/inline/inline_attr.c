// RUN: %inline_check %s

__attribute((always_inline))
void f()
{
}

__attribute((noinline))
int g()
{
	f();
	return 5;
}

main()
{
	return g();
}
