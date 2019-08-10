// RUN: %check --only %s

#include "printf.h"
main()
{
	printf("hi %d\n"); // CHECK: warning: too few arguments for argument (%d)
	printf("hi %d %s\n", 2); // CHECK: warning: too few arguments for argument (%s)
}
