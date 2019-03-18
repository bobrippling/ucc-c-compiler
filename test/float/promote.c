// -emit=print to avoid code gen
// RUN: %ucc -emit=print -fsyntax-only %s

#define CHK(exp, ty)                \
	_Static_assert(                   \
			__builtin_types_compatible_p( \
				__typeof(exp),                \
				ty),                        \
				"?")

f(float a, double b)
{
	CHK(a + b, double);
}

g(float a, long double b)
{
	CHK(a + b, long double);
}

h(double a, long double b)
{
	CHK(a + b, long double);
}
