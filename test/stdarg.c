#include <stdio.h>
#include <stdarg.h>

va(int a, ...)
{
	va_list l;

	va_start(l, a);

	printf("a=%d\n", a);

	while((a = va_arg(l, int)))
		printf("next=%d\n", a);

	va_end(l);
}

main()
{
	va(1, 2, 3, 0);
	return 0;
}
