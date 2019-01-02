// RUN: %check -e %s

f()
{
	float f = 2;
	f | 3.0; // CHECK: /error: binary \| between 'float' and 'double'/
}
