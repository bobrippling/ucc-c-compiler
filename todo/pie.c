static volatile int v_static = 2;
volatile int v_normal = 3;

int call_thru_got(void); // TODO: check with -fno-plt

int f()
{
	//volatile int q = 5;

	volatile int tmp = v_static;
	volatile int marker = 0;
	tmp += call_thru_got();
	marker = 1;
	tmp += v_normal;
	marker = 2;

	return tmp;
}

#ifndef __UCC__
void escape(int **p, int x)
{
	if(x)
		*p = &v_static;
	else
		*p = &v_normal;
}
#endif
