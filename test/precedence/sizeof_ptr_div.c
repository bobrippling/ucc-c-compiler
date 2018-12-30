// RUN: %check %s

int *p;

f()
{
	int sz = sizeof(p) / sizeof(*p); // CHECK: warning: division of sizeof(int *) - did you mean sizoef(array)?
}
