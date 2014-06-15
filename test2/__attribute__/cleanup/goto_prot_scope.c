// RUN: %ocheck 0 %s

int *pi, *pj;
int n;

cleanup(void *p)
{
	int *expected;

	switch(n){
		case 0: expected = pj; break;
		case 1: expected = pi; break;
		default: abort();
	}

	if(p != expected)
		abort();

	n++;
}
#define cleanup __attribute((cleanup(cleanup)))

f()
{
	cleanup int i = 0;
	goto after;

	cleanup int j = 1;
after:

	pi = &i;
	pj = &j;

	return 0;
}

main()
{
	f();
	if(n != 2)
		abort();
	return 0;
}
