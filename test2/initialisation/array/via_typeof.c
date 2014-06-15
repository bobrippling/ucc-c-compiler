// RUN: %layout_check %s
// RUN: %ucc -Xprint %s | grep -Fi "(aka 'struct A[3]')" >/dev/null

struct A
{
	int i, j;
} ent1[3];

__typeof(ent1) ent2;
