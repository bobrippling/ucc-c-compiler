// RUN: %ocheck 0 %s
int *pr;
int skipped;

clean(int *p)
{
	*pr = *p;
}

setup(int *r)
{
	pr = r;
}

g(){ return 5; }
h(){ return 12; }

f(int a)
{
	int r;

	setup(&r);

	{
		int __attribute((cleanup(clean))) i;
		switch(a){
			case 0:
				i = g();
				goto end;
			case 1:
				i = g() + g();
				goto end;
			case 2:
				i = 0;
				goto end;
			case 3:
				i = h();
				goto end;
			default:
				i = 6;
				break;
		}
	}

	skipped = 1;

end:
	return r;
}

main()
{
#include "../../ocheck-init.c"
	if(f(0) != 5 || skipped){
		_Noreturn void abort(void);
		abort();
	}
	if(f(1) != 10 || skipped){
		_Noreturn void abort(void);
		abort();
	}
	if(f(2) != 0 || skipped){
		_Noreturn void abort(void);
		abort();
	}
	if(f(3) != 12 || skipped){
		_Noreturn void abort(void);
		abort();
	}
	if(f(20) != 6 || !skipped){
		_Noreturn void abort(void);
		abort();
	}

	return 0;
}
