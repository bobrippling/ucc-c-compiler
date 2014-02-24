struct A
{
	int i, j;
};

void g(struct A *p);

void f(struct A *p)
{
	struct A a = *p;

	g(&(struct A){*p}); // bad
}
