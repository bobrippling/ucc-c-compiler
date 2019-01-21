// RUN: %check %s

struct A
{
	char buf[3];
	int i;
};

void g(void);

void f(struct A *p)
{
	if(&p->buf) // CHECK: warning: address of lvalue (char[3]) is always true
		g();

	if(&*p) // CHECK: warning: address of lvalue (struct A) is always true
		g();
}
