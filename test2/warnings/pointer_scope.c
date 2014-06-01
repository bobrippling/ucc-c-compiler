// RUN: %check %s -Wslow

int *q;

f()
{
	int i, *p;

	p = &i; // CHECK: !/warn/
}

main()
{
	int *p;

	{
		int i = 3;
		p = &i; // CHECK: warning: assigning address of 'i' to more-scoped pointer
	}

	int j;
	q = &j; // CHECK: warning: assigning address of 'j' to more-scoped pointer

	p = &j; // CHECK: !/warn/

	return *p;
}
