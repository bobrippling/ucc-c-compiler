#include <stdarg.h>

struct A
{
	unsigned length;
	int vals[];
};

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
	for(int j = 0; j < i; j++)
		r->vals[j] = list[j];

	r->length = i;

	return r;
}

print(struct A *p)
{
	for(int i = 0; i < p->length; i++)
		printf("%d\n", p->vals[i]);
}

main()
{
	print(&a);
	print(make(1, 2, 3, 4, 5, 0));
}
