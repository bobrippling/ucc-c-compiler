// RUN: %ucc -fsyntax-only %s
main()
{
	const int ki = 3;

	_Static_assert(
			__builtin_types_compatible_p(
				__typeof(ki),
				const int),
			"yo");
}
