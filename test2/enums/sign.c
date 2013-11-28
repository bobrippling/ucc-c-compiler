// RUN: %ucc -fsyntax-only %s

f(int *);

main()
{
	enum E // type = compiler specific
	{
		A = -1, // type = int
		B = -2
	};

	_Static_assert(
			__builtin_types_compatible_p(
				typeof(A),
				int),
			"enum mem -> int");
}
