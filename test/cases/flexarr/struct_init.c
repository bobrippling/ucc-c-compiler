// RUN: %ocheck 0 %s
int strcmp(const char *s1, const char *s2);
void abort(void) __attribute__((noreturn));

struct A
{
	int n;
	// pad of 4
	struct Ent
	{
		char *nam;
		int type;
	} ents[];
};

check(struct Ent *p, char *s, int n)
{
	if(strcmp(p->nam, s))
		abort();
	if(p->type != n)
		abort();
}

main()
{
	static struct A x = {
		2,
		{
			{ "hi", 5 },
			{ "yo", 2 },
		}
	};

	if(x.n != 2)
		abort();

	check(&x.ents[0], "hi", 5);
	check(&x.ents[1], "yo", 2);

	if(sizeof(x) != 8)
		abort();

	return 0;
}
