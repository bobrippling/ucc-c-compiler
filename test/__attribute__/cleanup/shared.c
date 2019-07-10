// RUN: %ocheck 0 %s

extern void *malloc(int);
extern void free(void *);

int freed;

struct A
{
	unsigned retains;
	void (*dealloc)(struct A *);
	char *mem;
};

void decref_a(struct A **pp)
{
	struct A *p = *pp;

	if(--p->retains)
		return;

	p->dealloc(p);
	free(p);
}

typedef __attribute((cleanup(decref_a))) struct A shared_a;

struct A *f()
{
	shared_a *p = malloc(sizeof *p);

	p->retains = 2; /* this and caller */

	return p;
}

void show_a_dealloc(struct A *p)
{
	freed = 1;
}

g()
{
	shared_a *p = f();
	p->dealloc = show_a_dealloc;
}

main()
{
	g();
	if(!freed){
		_Noreturn void abort();
		abort();
	}
	return 0;
}
