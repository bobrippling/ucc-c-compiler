// RUN: %check --prefix=normal %s -fno-short-enums
// RUN: %check --prefix=short  %s -fshort-enums

__builtin_va_list typedef va_list;

enum A{X};

int f(va_list l)
{
	return __builtin_va_arg(l, enum A); // CHECK-normal: warning: va_arg(..., enum A) has undefined behaviour when enums are short - promote to int
	// CHECK-short: ^ warning: va_arg(..., enum A) has undefined behaviour - promote to int
}
