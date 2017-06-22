// RUN: %check %s

void g(int *_Nonnull);

void f(int *_Nullable p)
{
	g(p); // CHECK: warning: mismatching types, argument 1 to g
}
