// RUN: %ucc -fsyntax-only %s

void f();
void f()
{
}

void g();
void g(void)
{
}

void h(void);
void h()
{
}

void i(void);
void i(void)
{
}

void main()
{
	f();
	g();
	h();
}
