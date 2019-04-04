void reg(void (*p)())
{
	p(1,2,3);
}

void mem(void (**p)())
{
	(*p)(1,2,3);
}

int f(int *p, int i)
{
#ifndef __HAVE_BUILTIN_SPECULATION_SAFE_VALUE
#error not defined
#endif
	return p[__builtin_speculation_safe_value(i /*, failvalue*/)];
}
