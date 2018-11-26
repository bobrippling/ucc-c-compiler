// RUN: %check -e %s

struct A
{
	__auto_type x = 3; // CHECK: error: member x is initialised
};

f(x)
	__auto_type x = 3; // CHECK: error: parameter "x" is initialised
{
	return x;
}
