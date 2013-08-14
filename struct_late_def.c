struct A;

int f()
{
	struct A a;
	a.i = 2;
	return a.i;
}

struct A
{
	int i;
};
