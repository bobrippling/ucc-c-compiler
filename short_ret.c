struct A
{
	int a, b, c;
};

struct A f(void)
{
	return (struct A){ 1, 2, 3 };
}
