// RUN: %ucc -o %t %s && %t

struct A
{
	char *a, *b;
};

struct B
{
	long a;
	char *b;
};

struct A a[] =
{
	{
		"",
		((char *)&((struct B){ 1, (char *)1 }))
	}
};

int main()
{
	struct B *pt3 = (struct B *)a[0].b;
	return pt3->b == (char *)1 ? 0 : 1;
}
