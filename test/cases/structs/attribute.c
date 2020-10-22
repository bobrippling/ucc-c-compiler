// RUN: %check %s

struct A
{
	int i;
};

int f(struct __attribute((noderef)) A *p) // CHECK: warning: cannot add attributes to already-defined type
{
	return p->i;
}
