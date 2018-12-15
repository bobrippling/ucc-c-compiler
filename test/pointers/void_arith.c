// RUN: %check %s
f(void *p)
{
	p++; // CHECK: /warning: arithmetic on void pointer/
}
