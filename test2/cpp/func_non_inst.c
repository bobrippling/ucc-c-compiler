// RUN: %check %s -Wall
// RUN: %ocheck 1 %s

#define f(x) x+1

(f)(int i) // CHECK: /warning: ignoring non-function instance of f/
{
	return i - 1;
}

main()
{
	return (f)(2); // CHECK: /warning: ignoring non-function instance of f/
}
