// RUN: %ucc %s

#include "printf.h"
#define NULL (void *)0

main()
{
	printf(NULL);
	printf(0);
}
