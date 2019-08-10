// RUN: %check --only %s

#include "printf.h"

int main()
{
	printf("hello %d\n", "yo"); // CHECK: warning: %d expects a 'int' argument, not 'char *'
	printf("hello %s\n", 2); // CHECK: warning: %s expects a 'char *' argument, not 'int'
}
