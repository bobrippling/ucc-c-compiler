// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

struct A
{
	struct A *next;
};

struct A *next(struct A *p)
{
	// we are casting this to void and messing with the phi-logic
	return p ? p->next : 0;
}

struct A *current(struct A *p)
{
	return p ? : 0;
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

	if(current(&self) != &self)
		abort();
	if(current(&a) != &a)
		abort();
	if(current(&b) != &b)
		abort();
	if(current(0) != 0)
		abort();

	return 0;
}
