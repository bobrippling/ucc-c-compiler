// RUN: %check -e %s

f()
{
	float x = 1.f;
	2 << x; // CHECK: /error: binary << between 'int' and 'float'/
}
