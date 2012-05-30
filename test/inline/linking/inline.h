#include <stdio.h>

#define HI() \
	printf("hi from %s\n", __func__)

static inline int x()
{
	HI();
}

int a();

extern int /*inline*/ z();
extern int   inline   y();
