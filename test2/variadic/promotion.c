f(int a, ...)
{
	__builtin_va_list l;
	__builtin_va_start(l, a);
	return __builtin_va_arg(l, short); // undefined
}
