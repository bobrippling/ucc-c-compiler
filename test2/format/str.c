// RUN: %ucc %s
// RUN: %check %s

#include "printf.h"

main()
{
	printf("%s\n", "hi"); // CHECK: !/warn/
}
