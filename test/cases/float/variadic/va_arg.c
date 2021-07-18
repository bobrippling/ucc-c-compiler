// RUN: %check %s

void f(int a, ...)
{
	__builtin_va_list l;
	__builtin_va_arg(l, float); // CHECK: /warning: va_arg\(..., float\) has undefined behaviour - promote to double/
}
