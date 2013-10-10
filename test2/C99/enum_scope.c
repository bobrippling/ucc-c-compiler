// RUN: %check -e %s -std=c99
main()
{
	// anon enum below is scoped only to the if in C99
	if(5 > (enum { a, b })1)
		return a;
	return b; // CHECK: /error: b not in scope/
}
