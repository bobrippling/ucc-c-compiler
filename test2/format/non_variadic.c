// RUN: %check %s

f(char *) __attribute__((format(printf, 1, 2))); // CHECK: warning: variadic function required

main()
{
	f(0);
}
