// RUN: %check %s

#include "printf.h"

main()
{
	printf("%ld\n", 5); // CHECK: warning: format %ld expects 'long' argument (got int)
	printf("%ld\n", (long)2);
	printf("%d\n", (long)3);
}
