// RUN: %check -e %s

g(enum B);

enum B
{
	B1, B2, B3
};

main()
{
	g(B2); // CHECK: /error: implicit cast to incomplete type enum B/
}
