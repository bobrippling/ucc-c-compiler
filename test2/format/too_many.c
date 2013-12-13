// RUN: %ucc %s
// RUN: %check %s

#include "printf.h"
main()
{
	printf("hi %d\n", 1, 2); // CHECK: /warning: too many arguments for format/
	printf("hi %d %s\n", 2, "hi", 3); // CHECK: /warning: too many arguments for format/
}
