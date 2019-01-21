// RUN: %ucc %s
// RUN: %check %s

#include "printf.h"
main()
{
	printf("hi %d\n", 5); // CHECK: !/warn/
}
