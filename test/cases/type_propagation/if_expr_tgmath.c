// RUN: %ucc -fsyntax-only %s

float cosf(float);
double cos(double);
long double cosl(long double);
#define __real__(X) (X)
#define _Complex

float _Complex ccosf(float);
double _Complex ccos(double);
long double _Complex ccosl(long double);

#define __isdouble(X)    sizeof(X) == sizeof(double)
#define __isldouble(X)   sizeof(X) == sizeof(long double)
#define __iscdouble(X)   sizeof(X) == sizeof(_Complex double)
#define __iscldouble(X)  sizeof(X) == sizeof(_Complex long double)

#define __isreal(X)      sizeof(X) == sizeof(__real__(X))
#define __isintegral(X)  (__typeof(X))1.1 == 1

#define __typeswitch_1(B, T) (1 ? (T *)0 : (void *)!(B)) // B ? T*    : void*
#define __typeswitch_2(B, T) (1 ? (T *)0 : (void *) (B)) // B ? void* : T*
#define __typeswitch(B, T, U) __typeof(*(1 ? (__typeof(__typeswitch_1(B, T)))0 : (__typeof(__typeswitch_2(B, U)))0))

#define __targettype(X) \
		__typeswitch( \
			__isreal(X), \
			__typeswitch( \
				(__isdouble(X) || __isintegral(X)), \
				double, \
				__typeswitch( \
					__isldouble(X), \
					long double, \
					float \
				) \
			), \
			__typeswitch( \
				__iscdouble(X), \
				_Complex double, \
				__typeswitch( \
					__iscldouble(X), \
					_Complex long double, \
					_Complex float \
				) \
			) \
		)

#define cos(X) ({ \
	__targettype(X) __ret; \
	if(__isreal(X)) \
		if(__isdouble(X) || __isintegral(X)) \
			__ret = (cos)(X); \
		else if(__isldouble(X)) \
			__ret = cosl(X); \
		else \
			__ret = cosf(X); \
	else \
		if(__iscdouble(X)) \
			__ret = ccos(X); \
		else if(__iscldouble(X)) \
			__ret = ccosl(X); \
		else \
			__ret = ccosf(X); \
	__ret; \
})

#define assert_expr_type(E, T) \
	_Static_assert(_Generic((E), T: 1))

assert_expr_type(cos(3), double);
assert_expr_type(cos(3.2), double);
assert_expr_type(cos(3.2f), float);
assert_expr_type(cos(1.0L), long double);

// TODO
// assert_expr_type(cos(3 + 1i), _Complex double);
// assert_expr_type(cos(3.2 - 2i), _Complex double);
// assert_expr_type(cos(3.2f + 0i), _Complex float);
// assert_expr_type(cos(1.0L - 0i), _Complex long double);
