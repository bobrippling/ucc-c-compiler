// RUN: %archgen %s 'x86,x86_64:movl (%%rax), %%eax' -DFIRST
// RUN: %archgen %s 'x86,x86_64:movl 4(%%rcx), %%ecx' -DSECOND

#ifdef FIRST

f(int *p)
{
	return *p;
	// should see that p isn't used after/.retains==1 and not create a new reg,
	// but re-use the current
}

#elif defined(SECOND)

struct A
{
	int i, j;
};

g(struct A *p)
{
	return p->i + p->j;
}

#else
#  error neither
#endif
