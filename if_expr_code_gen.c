// RUN: %ocheck 0 %s

struct A
{
	struct A *next;
};

struct A *next(struct A *p)
{
	// we are casting this to void and messing with the phi-logic
	return p ? p->next : 0;
}

main()
{
	struct A self = { &self };
	struct A a = { 0 }, b = { &a };

	if(next(&self) != &self)
		abort();
	if(next(&a) != 0)
		abort();
	if(next(&b) != &a)
		abort();
	if(next(0) != 0)
		abort();

	return 0;
}
