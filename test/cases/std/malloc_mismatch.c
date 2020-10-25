// RUN: %check %s

void *malloc(unsigned long);

main()
{
	int *p = malloc(3 * sizeof(int)); // CHECK: !/warn.*assignment.*malloc/
	char *a = malloc(sizeof(*p)); // CHECK: warning: malloc assignment with different types 'char *' and 'int *'
	char *b = malloc(sizeof(*b)); // CHECK: !/warn.*assignment.*malloc/
	void *v = malloc(sizeof(int)); // CHECK: !/warn.*assignment.*malloc/
}
