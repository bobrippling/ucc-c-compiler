// RUN: %check %s -w -Wattr-noderef -Wmismatching-types

#define userd(t) t __attribute((noderef))

f(userd(int) *p)
{
	int *local = p; // CHECK: warning: mismatching nested types

	int __attribute((noderef)) *x = p; // CHECK: !/warn/

	__auto_type abc = &*p; // CHECK: warning: dereference of noderef expression

	return *p // CHECK: warning: dereference of noderef
		+ *local;
}

main()
{
	int i = 3;

	int got = f(&i); // CHECK: !/warn/

	int __attribute((noderef)) *add_attr = &i; // CHECK: !/warn/

	add_attr[2]; // CHECK: warning: dereference of noderef

	return got;
}

struct A { int i; };

void g(userd(struct A) *p)
{
	__auto_type st = *p;

	__auto_type x = &p->i;

	*x; // CHECK: warning: dereference of noderef
}
