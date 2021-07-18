// RUN: %check -e %s

struct A
{
  int a : 2;
  int b : 3;
};

bf = __builtin_offsetof(struct A, b); // CHECK: error: offsetof() into bitfield

struct B
{
	struct A a;
};

bf2 = __builtin_offsetof(struct B, a.b); // CHECK: error: offsetof() into bitfield
