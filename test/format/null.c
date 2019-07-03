// RUN: %check --only %s

#include "printf.h"
#define NULL (void *)0

main()
{
	printf(NULL); // CHECK: warning: format argument isn't a string constant
	printf(0); // CHECK: warning: format argument isn't a string constant
}
