struct A
{
	long a, b, c, d;
};

f(volatile struct A *p)
{
	*p;
}
