struct A
{
	int i;
};

int main(void)
{
	struct B
	{
		struct A;
		struct A a;
	};

	struct B b;

	b.a.i = 3;

	return b.a.i;
}
