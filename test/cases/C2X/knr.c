// RUN: %check --only -e %s -std=c2x

void f();

void g(a) // CHECK: error: old-style functions have been removed in C2X and later
	int a;
{
}

int main()
{
	f(1); // CHECK: error: too many arguments to function f (got 1, need 0)
}
