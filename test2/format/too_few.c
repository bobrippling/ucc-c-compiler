// RUN: %ucc %s
// RUN: %check %s

#include "printf.h"
main()
{
	printf("hi %d\n"); // CHECK: /warning: too few arguments for format \(%d\)/
	printf("hi %d %s\n", 2); // CHECK: /warning: too few arguments for format \(%s\)/
}
