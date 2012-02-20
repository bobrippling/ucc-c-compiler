#include <assert.h>

printf();

x()
{
	static int i = 0;
	printf("%d\n", ++i);
}

int (*getptr())()
{
	return x;
}

int (*getptr_addr())()
{
	return &x;
}

main()
{
	int (*p)() = getptr(), (*q)();

	q = getptr_addr();

	p();
	q();
	getptr()();
	getptr_addr()();

	(*p)();
	(*q)();
	(*getptr())();
	(*getptr_addr())();

	assert(i == 8);

	return 0;
}
