// RUN: %check -e %s

struct A
{
	int q;
} t8;

_Static_assert(__alignof__(t8), "");

f()
{
	struct t8 t8 = { 0,0,1 }; // CHECK: error: struct t8 is incomplete
}
