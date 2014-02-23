// RUN: %ucc -fsyntax-only %s
main()
{
	char x[] = "123";
	// `x' had been folded but not init-folded,
	// so its type is still char[], not char[4],
	// which lead to sizeof saying it's incomplete
	char y[sizeof x];
	// fixed by init-folding once we have a decl

	_Static_assert(sizeof y == 4, "bad size");
}
