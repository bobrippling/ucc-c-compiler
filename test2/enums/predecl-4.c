// RUN: %check -e %s

f(enum A *p)
{
	p++; // CHECK: /error: arithmetic on pointer to incomplete type enum A/
}
