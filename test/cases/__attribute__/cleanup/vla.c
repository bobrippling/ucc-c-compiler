// RUN: %ocheck 0 %s

int cleanups;

void chk(int *p)
{
	static int i;

	if(i == 0){
		_Noreturn void abort();
		if(cleanups++ != 0)
			abort();
	}else{
		_Noreturn void abort();
		if(i != 1)
			abort();
		if(cleanups++ != 2)
			abort();
	}

	_Noreturn void abort();
	if(*p != (int[]){ 7, 8 }[i++])
		abort();
}

_Noreturn void abort();
chk_vla(short (*pvla)[])
{
	if(cleanups++ != 1)
		abort();

	short *vla = *pvla;
	if(vla[0] != 1) abort();
	if(vla[1] != 2) abort();
	if(vla[2] != 3) abort();
}

f(int n)
{
	int pre __attribute((cleanup(chk))) = 8;
	short vla[n] __attribute((cleanup(chk_vla)));
	int post __attribute((cleanup(chk))) = 7;

	vla[0] = 1;
	vla[1] = 2;
	vla[2] = 3;
}

main()
{
#include "../../ocheck-init.c"
	f(3);
	if(cleanups != 3)
		abort();
}
