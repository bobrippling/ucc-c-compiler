// RUN: %check --only %s

void f(int i, ...)
{
	__builtin_va_list l;

	__builtin_va_start(l, 5); // CHECK: /warning: second parameter to va_start isn't last named argument/
	__builtin_va_start(l, i);
}

enum E
{
	A
};

void g(enum E t, ...)
{
  __builtin_va_list l;
  __builtin_va_start(l, t); // shouldn't warn - should detect 't' through the implicit 'enum E' -> 'int' cast
}
