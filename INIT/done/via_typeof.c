// RUN: %ucc -c %s
// RUN: %ucc -Xprint %s | grep -Fi "(aka 'struct A [3]')"

struct A
{
	int i, j;
} x[3];

typeof(x) p;
