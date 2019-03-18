// RUN: %check --only %s

#include "printf.h"

main()
{
	printf("%ld\n", 5); // CHECK: warning: %ld expects a 'long' argument, not 'int'
	printf("%ld\n", (long)2);
	printf("%d\n", (long)3); // CHECK: warning: %d expects a 'int' argument, not 'long'
}
