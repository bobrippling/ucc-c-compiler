// RUN: %ucc -target x86_64-linux -S -o- %s -DFIRST  | grep 'movl (%%rax), %%eax'
// RUN: %ucc -target x86_64-linux -S -o- %s -DSECOND | grep 'movl 4(%%rcx), %%ecx'

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
