// RUN: %check -e %s

f()
{
	~1.0f; // CHECK: /error: unary ~ applied to type 'float'/
}
