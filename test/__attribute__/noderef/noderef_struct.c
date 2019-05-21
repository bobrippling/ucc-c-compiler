// RUN: %check --only %s

struct A { int i; };

int g(struct A __attribute((noderef)) *p)
{
	__auto_type st = *p; // CHECK: warning: dereference of noderef expression

	__auto_type x = &p->i; // CHECK: !/warning/

	int *local = &p->i; // CHECK: warning: mismatching nested types, initialisation
	// CHECK: ^ note: 'int *' vs 'int __attribute__((noderef)) *'

	return *x; // CHECK: warning: dereference of noderef expression
	// CHECK: ^ ! /warning: mismatching.*return/
}
