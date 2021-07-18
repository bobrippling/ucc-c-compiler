// RUN: %check -e %s

f(int) // CHECK: error: argument 1 in "f" is unnamed
{
	return a; // CHECK: error: undeclared identifier "a"
}

main()
{
	return f(3, 2); // CHECK: error: too many arguments to function f (got 2, need 1)
}
