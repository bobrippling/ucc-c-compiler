// RUN: %check --only %s

struct A
{
	char buf[3];
	int i;
};

void g(void);

void tautology(struct A *p)
{
	if(&p->buf) // CHECK: warning: address of lvalue (char[3]) is always true
		g();

	if(&*p) // CHECK: warning: address of lvalue (struct A) is always true
		g();
}

// -------------------------------------------

void take_any(void *);

void func_and_array_addressable()
{
	int f();
	int x[2];

	take_any(&f); // CHECK: /warning: implicit cast/
	take_any(&x);
}

// -------------------------------------------

struct Incomplete;

void *f(struct Incomplete *p)
{
	return &*p;
}
