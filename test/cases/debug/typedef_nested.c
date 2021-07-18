// RUN: %ucc -g -o %t %s

void f();

void g()
{
	typedef struct A *P;

	struct A
	{
		P p; // nested typedef was causing debug code gen assertion failures
	};

	P cinfo = 0;

	f(cinfo->p);
}

main()
{
}

void f()
{
}
