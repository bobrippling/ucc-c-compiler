#include "inline.h"

void a()
{
	HI();
}

extern void inline z()
{
	HI();
	printf("z calling x\n");
	x();
}

extern void inline y()
{
	HI();
}
