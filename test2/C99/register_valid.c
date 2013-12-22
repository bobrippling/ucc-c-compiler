// RUN: %ucc -fsyntax-only %s

// C11, 6.7.6.3, 2 The only storage-class specifier that shall occur in a
// parameter declaration is register.

f(register int i);

main()
{
	f(1);

	for(register int i = 0; i < 10; i++)
		;
}
