// RUN: %check -e %s
main()
{
	if(5 > (enum { a, b })1)
		return a;
	return b; // CHECK: /error: b not in scope/
}
