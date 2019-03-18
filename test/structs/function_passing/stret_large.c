// RUN: %ocheck 0 %s
// RUN: %ocheck 0 %s -fstack-protector-all

struct A
{
	int x, y, z;
	int e, f;
};

void clobber_args()
{
}

struct A f(void)
{
	clobber_args(0, 1, 2); // clobber the stret pointer
	return (struct A){ 1, 2, 3, 4, 5};
}

int main()
{
	struct A a;

	a = f();

	if(a.x != 1
	|| a.y != 2
	|| a.z != 3
	|| a.e != 4
	|| a.f != 5)
	{
		abort();
	}

	return 0;
}
