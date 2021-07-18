// RUN: %check %s -Wsign-compare

f(signed s, unsigned u)
{
	(void)(s + u); // CHECK: !/warn/
	(void)(s + 1u); // CHECK: !/warn/
	(void)(u + 1); // CHECK: !/warn/

	(void)(s == u); // CHECK: warning: signed and unsigned types in '=='
	return 1 ? s : u; // CHECK: warning: signed and unsigned types in '?:'
}
