#include <stdio.h>

#define HI() \
	printf("hi from %s\n", __func__)

static inline void x()
{
	HI();
}

void a();

extern void /*inline*/ z();
extern void   inline   y();
