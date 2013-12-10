typedef __builtin_va_list va_list;
#define va_start(l, p) __builtin_va_start(l, p)
#define va_end(l)      __builtin_va_end(l)
#define va_arg(l, ty)  __builtin_va_arg(l, ty)

enum { INT, FP, END };

double f(int ty, ...)
#if defined IMPL || defined ALL
{
	va_list l;
	double t = 0;
	va_start(l, ty);
	for(;;){
		printf("ty=%d\n", ty);
		switch(ty){
			case INT: t += va_arg(l, int); break;
			case FP:  t += va_arg(l, double); break;
			case END: goto done;
		}
		ty = va_arg(l, int);
		printf("hello %f\n", t);
	}
done:
	va_end(l);
	return t;
}
#else
;
#endif

#if !defined IMPL || defined ALL
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

	if(x != 56)
		abort();

	return 0;
}
#endif
