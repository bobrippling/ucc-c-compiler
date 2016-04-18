// RUN: %ocheck 0 %s

int n;

void clean_int(int *x)
{
	n++;
}

g(){}
h(){}

f(int i)
{
	switch(i){
		int xc __attribute((cleanup(clean_int))) = 56;
		case 2:
		case 5:
			g();
			return;
		case 9:
			h();
	}

	return i + 2;
}

main()
{
	_Noreturn void abort();
	f(0);
	if(n) abort();
	f(2);
	if(n != 1) abort();
	f(9);
	if(n != 2) abort();
	f(10);
	if(n != 2) abort();
	return 0;
}
