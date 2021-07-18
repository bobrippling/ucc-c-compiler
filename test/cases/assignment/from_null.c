// RUN: %check %s
enum A
{
	ZERO
};
void *p = (enum A)0; // CHECK: !/warn/
void *q = ZERO; // CHECK: !/warn/
