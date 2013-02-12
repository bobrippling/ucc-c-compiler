f(int a, ...)
{
	__builtin_va_list l;

	__builtin_va_start(l, a);
}

normal(int a, int b)
{
	return a+b;
}
