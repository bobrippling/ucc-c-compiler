__attribute((always_inline))
void f()
{
}

__attribute((noinline))
int g()
{
	return 5;
}

main()
{
	return g();
}
