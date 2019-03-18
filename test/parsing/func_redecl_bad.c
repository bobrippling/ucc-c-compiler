// RUN: %check -e %s

void f(void); // CHECK: note: previous definition

f() // CHECK: error: mismatching definitions of "f"
{
}

main()
{
	// ensure we don't abort on this:
	f(1); // CHECK: error: too many arguments to function f (got 1, need 0)
}
