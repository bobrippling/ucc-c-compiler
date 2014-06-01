// RUN: %check %s -Wslow

int *q;

main()
{
	int *p;

	{
		int i = 3;
		p = &i; // CHECK: warning: assigning address of 'i' to more-scoped pointer
	}

	int j;
	q = &j; // CHECK: warning: assigning address of 'j' to more-scoped pointer

	return *p;
}
