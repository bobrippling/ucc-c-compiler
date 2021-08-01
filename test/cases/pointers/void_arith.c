// RUN: %check %s

f(void *p, void *q)
{
	p++; // CHECK: /warning: arithmetic on void pointer/

	q - p; // CHECK: /warning: arithmetic on void pointer/
}
