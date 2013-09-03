#include "fp.h"

f(union fp *u)
{
	u->bits.sign = 0;
	u->bits.exponent = 0b01111100;
	u->bits.fract = 1 << 21;
}
