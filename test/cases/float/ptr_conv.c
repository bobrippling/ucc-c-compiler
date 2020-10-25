// RUN: %check -e %s

f(int *);
g(float);

main()
{
	int *pt = (void *)0;
	float fp = 0;

	f(fp); // CHECK: /error: mismatching types/
	f(pt); // CHECK: !/error/
	g(fp); // CHECK: !/error/
	g(pt); // CHECK: /error: mismatching types/
}
