// RUN: %ucc -fsyntax-only %s

main()
{
	int a[4];
	__typeof(a) b;
	__typeof(int[4]) c;

	_Static_assert(sizeof(a) + sizeof(b) + sizeof(c) == 3 * 4 * sizeof(int),
			"array not propagated in __typeof()");
}
