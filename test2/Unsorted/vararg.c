#include <stdarg.h>
#include <assert.h>

va(int a, ...)
{
	va_list l;
	int t = 0;

	va_start(l, a);
	while(a){
		t += a;
		a = va_arg(l, int);
	}
	va_end(l);
}

main()
{
	assert(va(0, 1) == 0);
	assert(va(1, 2, 3, 0) == 6);
	assert(va(4, 5, 6, 7, -1, 0) == 21);
}
