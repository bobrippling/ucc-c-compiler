// RUN: %ocheck 45 %s
//#include <stdarg.h>

typedef __builtin_va_list va_list;

f(int a, ...)
{
	va_list l;

	__builtin_va_start(l, 5);
	//vf(a, l);

	int t = 0;
	while(a != -1){
		//pd(a);
		t += a;
		a = __builtin_va_arg(l, int);
	}
	__builtin_va_end(l);
	return t;
}

/* stack layout of f:
 * 1- 8  rdi
 * 8-16  r9
 * 16-24  r8
 * 24-32  rcx
 * 32-40  rdx
 * 40-48  rsi
 * 48-56  AAAA // va_list::ptr1 - initialised to 48
 * 56-64  BBBB // va_list::ptr2 - initialised to frame-ptr + 8
 * 64-72  CCDD // va_list::i, va_list::j - both 0-initialised
 * 72-80  EE__ // t, <empty>
 *         ^76
 */

main()
{
	//return f(1, 2, 3, -1);
	return f(1, 2, 3, 4, 5, 6, 7, 8, 9, -1);
}
