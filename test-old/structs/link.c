abort();
printf();

#define NULL (void *)0

struct A
{
	int a;
	int used;
	struct A *next;
};

struct A a, b, c, d;

next()
{
#define RUSED(x)  \
	if(!x.used){    \
		x.used = 1;   \
		return &x;    \
	}

	RUSED(a);
	RUSED(b);
	RUSED(c);
	RUSED(d);
	abort();
}

add(struct A *p, int n)
{
	while(p->next)
		p = p->next;

	p->next = next();
	p = p->next;
	p->a = n;
}

walk(struct A *p)
{
	printf("%d\n", p->a);
	if(p->next)
		walk(p->next);
}

main()
{
	struct A *first = next();

	first->a = 1;
	add(first, 2);
	add(first, 3);
	add(first, 4);

	walk(first);
}
