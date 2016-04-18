// RUN: %check -e %s

void f(void);

void f() // old proto, should still get error on call because of (void) above
{
}

main()
{
	f(1); // CHECK: error: too many arguments to function f (got 1, need 0)
}
