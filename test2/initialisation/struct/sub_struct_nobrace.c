// RUN: %check -e %s

struct A
{
	int i;
} glob = { 5 };

f()
{
  struct A loc = { glob }; // CHECK: error: mismatching types, initialisation:
	// CHECK: ^ note: 'int' vs 'struct A'
  return loc.i;
}

g()
{
	(struct A){ glob }; // CHECK: error: mismatching types, initialisation:
	// CHECK: ^ note: 'int' vs 'struct A'
}
