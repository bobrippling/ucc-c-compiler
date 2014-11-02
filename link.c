struct A
{
};

inline void decref_a(struct A **pp)
{
	struct A *p = *pp;
}

typedef __attribute((cleanup(decref_a))) struct A shared_a;

inline struct A *f()
{
	shared_a *p = 0;
	return p;
}

main()
{
	shared_a *p = f();
}
