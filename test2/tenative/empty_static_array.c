// RUN: %check -e %s
f()
{
	static int ar2[]; // CHECK: /error: incomplete array size attempt/
}
