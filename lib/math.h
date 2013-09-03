#ifndef __MATH_H
#define __MATH_H

#define MATH_FUNC(f) float       f ## f(float);      \
                     double      f(     double);     \
                     long double f ## l(long double)

MATH_FUNC(sin);
MATH_FUNC(cos);
MATH_FUNC(tan);

MATH_FUNC(asin);
MATH_FUNC(acos);
MATH_FUNC(atan);

MATH_FUNC(sinh);
MATH_FUNC(cosh);
MATH_FUNC(tanh);

MATH_FUNC(pow);
MATH_FUNC(exp);

MATH_FUNC(log);
MATH_FUNC(log10);

MATH_FUNC(sqrt);

MATH_FUNC(fabs);
MATH_FUNC(round);
MATH_FUNC(ceil);
MATH_FUNC(floor);

#undef MATH_FUNC

#endif
