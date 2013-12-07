// RUN: %ucc -fsyntax-only %s

f(long long l, float f)
{
	_Static_assert(
			__builtin_types_compatible_p(
				__typeof(l * f),
				float),
			"long long not converted to float");

	return l * f; // should promote to float
}
