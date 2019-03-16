void reg(void (*p)())
{
	p(1,2,3);
}

void mem(void (**p)())
{
	(*p)(1,2,3);
}
