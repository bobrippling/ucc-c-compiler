// RUN: %check -e %s

f()
{
	1.2f << 3; // CHECK: /error: binary << between 'float' and 'int'/
}
