// RUN: %check %s
void *_new(int sz, int n);
void free(void *);

#define new(ty, n) (ty *)_new(sizeof(ty), n)
#define delete(x) free(x)

f(int [static 4]);

main()
{
	int *p = new(int, 4);
	f(p); // CHECK: !/warn/
	delete(p);
}
