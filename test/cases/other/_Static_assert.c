// RUN: %ucc -fsyntax-only %s

_Static_assert(sizeof(void *) == sizeof(int *), "");

main()
{
	int i;

	_Static_assert(sizeof(i) == sizeof(int), "");

	return 0;
}
