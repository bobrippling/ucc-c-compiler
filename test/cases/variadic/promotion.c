// RUN: %check %s
f(int a, ...)
{
	__builtin_va_list l;
	__builtin_va_start(l, a);
	return __builtin_va_arg(l, short); // CHECK: /warning: va_arg\(\.\.\., short\) has undefined behaviour - promote to int/
}

main(){}
