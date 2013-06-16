// RUN: %ucc %s
// RUN: %check %s

#include "printf.h"
main()
{
	printf("hello %d\n", "yo"); // CHECK: /warning: format %d expects integral argument/
	printf("hello %s\n", 2); // CHECK: /warning: format %s expects 'char \*' argument/
}

