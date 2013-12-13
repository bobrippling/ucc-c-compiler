// RUN: %ucc -fsyntax-only %s
unsigned f(unsigned char c)
{
	_Static_assert(
			__builtin_types_compatible_p(
				typeof(c << 24),
				signed int),
			"unsigned shifted by signed should be signed");

	_Static_assert(
			__builtin_types_compatible_p(
				typeof(c << 24U),
				int),
			"signed shifted by unsigned should be signed");

	_Static_assert(
			__builtin_types_compatible_p(
				typeof((unsigned)c << 24U),
				unsigned),
			"unsigned shifted by unsigned should be unsigned");

	return c << 24;
}
