// RUN: %check %s

#define nonnull __attribute((nonnull()))

void f(int *p nonnull)
{
	if(p) // CHECK: warning: testing a nonnull value
		;
}

void g(int *p) nonnull
{
	if(p) // CHECK: warning: testing a nonnull value
		;
}
