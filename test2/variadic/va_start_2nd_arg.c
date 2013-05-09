// RUN: %check %s

f(int i, ...)
{
	__builtin_va_list l;

	__builtin_va_start(l, 5); // CHECK: /warning: second parameter to va_start isn't last named argument/
	__builtin_va_start(l, i); // CHECK: !/warn/
}
