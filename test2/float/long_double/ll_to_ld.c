// RUN: %ucc -o %t %s
// RUN: %t | %output_check '-1.0'

#define __noinline __attribute__((noinline))

__noinline g(long double d)
{
	printf("%.1Lf\n", d);
}

__noinline f(unsigned long long ll, int sig)
{
	long double d;

	if(sig)
		d = (long long)ll;
	else
		d = ll;

	g(d);
}

main()
{
	f(-1, 1);
}
