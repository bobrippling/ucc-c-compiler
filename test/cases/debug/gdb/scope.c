// RUN: %debug_check %s | %stdoutcheck %s

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

// global a:
// STDOUT: a 5 41
// args, locals, etc:
// STDOUT: a<1> 5 30
// STDOUT: a<2> 25 28
// STDOUT: b 5 30
// STDOUT: c 5 30
// STDOUT: d 5 30
// STDOUT: e 5 30
// STDOUT: f 5 30
// STDOUT: fp 5 30
// STDOUT: g 5 30
// STDOUT: h 5 30
// STDOUT: i 5 30
// STDOUT: j 5 30
// STDOUT: k 5 30
// STDOUT: l 5 30
// STDOUT: local1 30 41
// STDOUT: local5 32 41
// STDOUT: m 5 30
// STDOUT: n 5 30
// STDOUT: pfp 15 28
// STDOUT: pt 5 30
// STDOUT: pt_local 5 30
