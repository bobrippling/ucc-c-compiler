// RUN: %check %s

void free(void *);

main()
{
	int x;
	free(&x); // CHECK: warning: free() of non-heap object

	char buf[3];
	free(buf); // CHECK: warning: free() of non-heap object

	int *p = 0;
	free(p); // CHECK: !/warn/

	struct A
	{
		int *x;
	} *a = 0;
	free(a->x); // CHECK: !/warn/

	char **ents = 0;
	free(ents); // CHECK: !/warn/
	free(&ents); // CHECK: warning: free() of non-heap object
}
