// RUN: %check --only %s

f(int, int, char *, ...) __attribute__((format(printf, 3, 3))); // CHECK: warning: variadic argument out of bounds (should be 4)

g(int, int, int, int, char *, ...)
	__attribute__((format(printf, 5, 4))); // CHECK: warning: variadic argument out of bounds (should be 6)

main()
{
	f(0, 0, "hi", 2);
	g(0, 0, 0, 0, "hi", 2);
}
