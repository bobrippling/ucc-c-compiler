#define NEST

struct Tim
{
	int i;
	int j;
#ifdef NEST
	struct
	{
		int w, x;
	} st;
#endif
};

set(struct Tim *tp)
{
#ifdef NEST
	tp->st->x = 3;
#else
	tp->j = 5;
#endif
}

main()
{
	struct Tim t;

	set(&t);

#ifdef NEST
	return t.st.x;
#else
	return t.j;
#endif
}
