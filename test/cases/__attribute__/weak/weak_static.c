// RUN: %check -e %s

static int static_weak __attribute((weak)); // CHECK: error: weak attribute on declaration without external linkage

main()
{
	int w __attribute((weak)); // CHECK: error: weak attribute on declaration without external linkage
	static int sw __attribute((weak)); // CHECK: error: weak attribute on declaration without external linkage
}
