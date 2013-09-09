main()
{
	const int ki = 3;

	_Static_assert(
			__builtin_types_compatible_p(
				typeof(ki),
				const int),
			"yo");
}
