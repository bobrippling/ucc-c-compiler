// RUN: %ucc -fsyntax-only %s

main()
{
	int a[4];
	typeof(a) b;
	typeof(int[4]) c;

	_Static_assert(sizeof(a) + sizeof(b) + sizeof(c) == 3 * 4 * sizeof(int),
			"array not propagated in typeof()");
}
