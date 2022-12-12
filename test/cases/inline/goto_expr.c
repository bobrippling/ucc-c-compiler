// RUN: %ocheck 0 %s -finline-functions -fno-semantic-interposition

__attribute((always_inline))
int f(int x)
{
	void *jmps[] = {
		&&a, &&b, &&c
	};

	goto *jmps[x];
b:
	return x + 3;
c:
	return x + 1;
a:
	x += 2;
	goto *&&b;
}

assert(int c)
{
	if(!c){
		extern void abort(void);
		abort();
	}
}

main()
{
#include "../ocheck-init.c"
	assert(f(0) == 5);
	assert(f(1) == 4);
	assert(f(2) == 3);

	return 0;
}
