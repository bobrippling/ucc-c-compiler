// RUN: %ucc -w -g -o %t %s
// RUN: %gdbcheck %t %s.cmds %s.expected

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
