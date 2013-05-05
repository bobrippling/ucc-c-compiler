// RUN: %ucc %s

_Static_assert(sizeof(void *) == sizeof(int *), "hi");

main()
{
	int i;

	_Static_assert(sizeof(i) == sizeof(int), "buh");

	return 0;
}
