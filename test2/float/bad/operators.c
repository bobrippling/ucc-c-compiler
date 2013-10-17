// RUN: %check -e %s

f(float a, float b)
{
	a % b;  // CHECK: /error:.*float/
	a ^ b;  // CHECK: /error:.*float/
	a | b;  // CHECK: /error:.*float/
	a & b;  // CHECK: /error:.*float/
	a * b;  // CHECK: !/error/
	a / b;  // CHECK: !/error/
	a + b;  // CHECK: !/error/
	a - b;  // CHECK: !/error/

	a || b; // CHECK: !/error/
	a && b; // CHECK: !/error/
	a << b; // CHECK: /error:.*float/
	a >> b; // CHECK: /error:.*float/
	~a;     // CHECK: /error:.*float/
	!a;     // CHECK: !/error/
	a == b; // CHECK: !/error/
	a != b; // CHECK: !/error/
	a <= b; // CHECK: !/error/
	a < b;  // CHECK: !/error/
	a >= b; // CHECK: !/error/
	a > b;  // CHECK: !/error/
}
