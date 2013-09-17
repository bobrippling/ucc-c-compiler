void store(_Atomic(int) *const p)
{
	*p = 3;
}

void inc_pre(_Atomic(int) *const p)
{
	(*p)++;
}

void inc_post(_Atomic(int) *const p)
{
	++*p;
}
