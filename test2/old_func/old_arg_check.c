// RUN: %check %s -Wall

f(a, b) // CHECK: warning: old-style function declaration
	int a, b; // here we can get information for a warning below; f()
{
}

g(); // CHECK: warning: old-style function declaration (needs "(void)")

main()
{
	f(f(), f()); // CHECK: warning: too few arguments to function f (got 0, need 2)
	g(g(), g()); // CHECK: !/warn/
}
