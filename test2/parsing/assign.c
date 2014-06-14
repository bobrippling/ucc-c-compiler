// RUN: %check -e %s

main()
{
	int *i = 0;
	while(*i == *i=5); // CHECK: /error: assignment to.*not an lvalue/
}
