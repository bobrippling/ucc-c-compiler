// RUN: %inline_check %s -fno-inline-functions

__attribute((always_inline))
int f()
{
	return 3;
}

main()
{
	return f(); // should be inlined despite the compiler argument
}
