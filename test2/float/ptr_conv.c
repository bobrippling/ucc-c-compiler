// RUN: %check -e %s

f(int *);
g(float);

main()
{
	int *pt = (void *)0;
	float fp = 0;

	f(fp); // CHECK: /error: implicit cast to pointer from floating type/
	f(pt); // CHECK: !/error/
	g(fp); // CHECK: !/error/
	g(pt); // CHECK: /error: implicit cast from pointer to floating type/
}
