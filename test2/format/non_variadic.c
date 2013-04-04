// RUN: ! %ucc %s
// RUN: %check %s

f(char *) __attribute__((format(printf, 1, 2))); // CHECK: /error: non variadic/

main()
{
	f(0);
}
