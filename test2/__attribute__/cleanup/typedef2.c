// RUN: %ucc -o %t %s

struct A
{
};

__attribute__((always_inline))
inline void decref_a(struct A **pp)
{
	struct A *p = *pp;
}

typedef __attribute((cleanup(decref_a))) struct A shared_a;

__attribute__((always_inline))
inline struct A *f()
{
	shared_a *p = 0;
	return p;
}

main()
{
	shared_a *p = f();
}
