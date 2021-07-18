// RUN: %check --only -e %s

union A
{
	int i;
};

void f(union A a) // CHECK: error: union arguments are not yet implemented
{
}
