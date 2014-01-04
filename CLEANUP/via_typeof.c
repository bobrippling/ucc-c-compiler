// RUN: %ucc -c %s
// RUN: %ucc -Xprint %s | grep -Fi "(aka 'struct A [3]')"

struct A
{
	int i, j;
} ent1[3];

typeof(ent1) ent2;
