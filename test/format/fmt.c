// RUN: %check --only %s

#include "printf.h"

typedef struct A T;
T *ext;

int main()
{
	printf("hi %d\n", 5);
	printf("%s\n", "hi");
	printf("%p\n", ext);
}
