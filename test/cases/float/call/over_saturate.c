// RUN: %ocheck 1 %s

//#include <stdarg.h>
typedef __builtin_va_list va_list;
#define va_start(l, p) __builtin_va_start(l, p)
#define va_end(l)      __builtin_va_end(l)
#define va_arg(l, ty)  __builtin_va_arg(l, ty)

enum ab { INT, FP, END };

double f(enum ab a, ...)
{
	double t = 0;
	va_list l;
	va_start(l, a);

	for(;;){
		switch(a){
			case END:
				goto fin;
			case FP:
				t += va_arg(l, double);
				break;
			case INT:
				t += va_arg(l, int);
				break;
		}
		a = va_arg(l, enum ab);
	}
fin:
	va_end(l);
	return t;
}

main()
{
	double x = f(
			INT, 1,
			FP,  1.0,
			INT, 2,
			FP,  2.0,
			INT, 3,
			FP,  3.0,
			INT, 4,
			FP,  4.0,
			INT, 5,
			FP,  5.0,
			INT, 6,
			FP,  6.0,
			INT, 7,
			FP,  7.0,
			END);

	return x == 56;
}
