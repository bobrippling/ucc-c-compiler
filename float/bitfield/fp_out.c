#include "fp.h"
main()
{
	union fp x;

	f(&x);

	printf("%f\n", x.f);
}
