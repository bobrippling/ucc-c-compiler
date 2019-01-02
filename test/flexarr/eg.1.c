// RUN: %ocheck 0 %s

typedef __builtin_va_list va_list;
#define va_start(l, f) __builtin_va_start(l, f)
#define va_arg(l, ty)  __builtin_va_arg(l, ty)
#define va_end(l)      __builtin_va_end(l)

struct A
{
	unsigned length;
	int vals[];
};

typedef unsigned long size_t;
extern void *malloc(size_t);
int memcmp(void const *, void const *, size_t);
void abort(void);

struct A a = { 2, { 1, 2 } };

struct A *make(int first, ...)
{
	va_list l;
	int list[10];
	int i = 0;

	va_start(l, first);
	do{
		list[i++] = first;
		first = va_arg(l, int);
	}while(first && i < 10);
	va_end(l);

	struct A *r = malloc(sizeof *r + i * sizeof r->vals[0]);

	_Static_assert((sizeof *r + sizeof r->vals[0]) == 8, "bad size");

	for(int j = 0; j < i; j++)
		r->vals[j] = list[j];

	r->length = i;

	return r;
}

main()
{
	if(a.length != 2 || a.vals[0] != 1 || a.vals[1] != 2)
		abort();

	struct A *p = make(1, 2, 3, 4, 5, 0);
	if(p->length != 5)
		abort();
	if(memcmp(p->vals, (__typeof(p->vals)){ 1, 2, 3, 4, 5 }, 5 * sizeof *p->vals))
		abort();

	return 0;
}
