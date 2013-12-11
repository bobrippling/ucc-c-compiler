// RUN: %check -e %s

enum A;

f(enum A a); // CHECK: !/error/

main()
{
	f(2); // CHECK: /error: implicit cast to incomplete type enum A/
}
