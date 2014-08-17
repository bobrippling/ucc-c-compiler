// RUN: %ocheck 3 %s

int *px, *pi;
int cleans = 0;

clean(int *p)
{
	int *target;
	switch(cleans++){
		case 0: target = pi; break;
		case 1: target = px; break;
		default: abort();
	}
	if(p != target)
		abort();

	*p = 1;
}

main()
{
	int x __attribute__((cleanup(clean))) = 3;

	px = &x;

	for(int i __attribute((cleanup(clean))) = 0; i < 3; i++)
		if(i == 0)
			pi = &i;

	return x;
}
