// RUN: %debug_check %s
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));
void abort(void) __attribute__((noreturn));

char a;

f(a,b,c,d,e,f,
	g,h,i,j,k,l,m,n,
	pt, fp)

	float g, h, i, j, k, l, m, n;

	char *pt;
	float fp;

{
	char *pt_local = pt + 1;

	printf("%p %f\n", pt, fp);

	float *pfp = &fp;
	if(*pfp != fp)
		abort();

	{
		extern char a;
		if(pt_local != &a + 1)
			abort();
	}
}

main()
{
	int local1 = 1;

	printf("&a = %p\n", &a);

	int local5 = 5;

	f(1,2,3,4,local5,6,
		1,2,3,4,5,6,7,8,
		&a, 3);
}
