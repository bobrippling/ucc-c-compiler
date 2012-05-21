//#define _Static_assert

_Static_assert(sizeof(void *) == sizeof(int *), "hi");

main()
{
	int i;

	_Static_assert(sizeof(i) == sizeof(int), "buh");
}
