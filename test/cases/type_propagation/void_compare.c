// RUN: %check %s

struct A;

void g(int);

void f(const struct A *pw)
{
	g(pw == (void *)0); // CHECK: !/warn.*const/
}
