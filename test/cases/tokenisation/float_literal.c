// RUN: %ucc -fsyntax-only %s

#define TYCHECK(t, e)            \
	_Static_assert(                \
			_Generic(e, t:1),          \
			"\"" #e "\" is not " #t);

TYCHECK(double, 1.23)
TYCHECK(double, 1.230)
TYCHECK(double, 123e-02)
TYCHECK(double, 123e-2)

TYCHECK(float, 1.23f)
TYCHECK(float, 1.230f)
TYCHECK(float, 123e-02f)
TYCHECK(float, 123e-2f)

TYCHECK(int, 0x5e2) // actually an int - 'e' isxdigit

TYCHECK(double, 05e3)
TYCHECK(double, 03e7)

TYCHECK(float, 05e3f)
TYCHECK(float, 03e7f)


TYCHECK(double, 3e01) // not octal => 30
_Static_assert(3e01 == 30, "");
TYCHECK(float, 3e01f)
_Static_assert(3e01f == 30, "");

// ldouble
TYCHECK(long double, 1.l)
TYCHECK(long double, 1e0L)

TYCHECK(float, .2f)
