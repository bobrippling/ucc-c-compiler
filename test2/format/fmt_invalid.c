// RUN: %ucc %s
// RUN: %check %s

f(int, int, char *, ...) __attribute__((format(printf, 3, 3))); // CHECK: /error: variadic argument out of bounds/
g(int, int, int, int, char *, ...) __attribute__((format(printf, 5, 4))); // CHECK: /error: format variadic argument before string/

main()
{
	f(0, 0, "hi", 2);
	g(0, 0, 0, 0, "hi", 2);
}
