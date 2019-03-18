// RUN: %check %s

void f(int *);
void g(void);

void f(int *f)
{
	if(f == (void *)0) // CHECK: !/warn/
		g();
}
