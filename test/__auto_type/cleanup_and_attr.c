// RUN: %ocheck 0 %s

// this checks attributes are correctly retained on __auto_type:s

int dints;

void dint(int *p)
{
	dints++;
}

void f()
{
	__attribute((cleanup(dint))) __auto_type x = 3;
	__attribute((cleanup(dint))) __auto_type y = 3;
}

void g()
{
	__attribute((cleanup(dint))) __auto_type x = 3;
	__attribute((cleanup(dint))) __auto_type y = 3;

	f();
}

main()
{
	g();

	if(dints != 4){
		_Noreturn void abort();
		abort();
	}

	return 0;
}
