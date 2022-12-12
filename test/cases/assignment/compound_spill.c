// RUN: %ocheck 0 %s

int f()
{
	return 3;
}

void fp(double *p)
{
	*p += f();
}

void inte(int *p)
{
	*p += f();
}

main()
{
#include "../ocheck-init.c"
	double d = 2;

	fp(&d);
	if(d != 5){
		_Noreturn void abort();
		abort();
	}

	int i = 7;
	inte(&i);
	if(i != 10){
		_Noreturn void abort();
		abort();
	}

	return 0;
}
