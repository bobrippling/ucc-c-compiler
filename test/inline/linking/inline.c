#include "inline.h"

int a()
{
	HI();
}

extern int inline z()
{
	HI();
	printf("z calling x\n");
	x();
}

extern int inline y()
{
	HI();
}
