#include <complex.h>
#include <math.h>

#define __IS_FP(x) !!((1?1:(x))/2)
#define __IS_CX(x) (__IS_FP(x) && sizeof(x) == sizeof((x)+I))
#define __IS_REAL(x) (__IS_FP(x) && 2*sizeof(x) == sizeof((x)+I))

#define __FLT(x) (__IS_REAL(x) && sizeof(x) == sizeof(float))
#define __LDBL(x) (__IS_REAL(x) && sizeof(x) == sizeof(long double) && sizeof(long double) != sizeof(double))

#define __FLTCX(x) (__IS_CX(x) && sizeof(x) == sizeof(float complex))
#define __DBLCX(x) (__IS_CX(x) && sizeof(x) == sizeof(double complex))
#define __LDBLCX(x) (__IS_CX(x) && sizeof(x) == sizeof(long double complex) && sizeof(long double) != sizeof(double))

#define __RETCAST_2(x, y) (__typeof__(*( \
	0 ? (__typeof__(0 ? (double *)0 : \
		(void *)!((!__IS_FP(x) || !__IS_FP(y)) && __FLT((x)+(y)+1.0f))))0 : \
	0 ? (__typeof__(0 ? (double complex *)0 : \
		(void *)!((!__IS_FP(x) || !__IS_FP(y)) && __FLTCX((x)+(y)))))0 : \
	    (__typeof__(0 ? (__typeof__((x)+(y)) *)0 : \
		(void *)((!__IS_FP(x) || !__IS_FP(y)) && (__FLT((x)+(y)+1.0f) || __FLTCX((x)+(y))))))0 )))


#define __tg_real_complex_pow(x, y) (__RETCAST_2(x, y)( \
	__FLTCX((x)+(y)) && __IS_FP(x) && __IS_FP(y) ? cpowf(x, y) : \
	__FLTCX((x)+(y)) ? cpow(x, y) : \
	__DBLCX((x)+(y)) ? cpow(x, y) : \
	__LDBLCX((x)+(y)) ? cpowl(x, y) : \
	__FLT(x) && __FLT(y) ? powf(x, y) : \
	__LDBL((x)+(y)) ? powl(x, y) : \
	pow(x, y) ))

#define xpow(x,y)        __tg_real_complex_pow((x), (y))


int main() {
  double x;
  x = xpow(2,2);
}
