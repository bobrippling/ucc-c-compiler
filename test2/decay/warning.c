// RUN: %check %s

main()
{
	char x[200];
	return sizeof(0, x); // CHECK: array-argument evaluates to sizeof(char *), not sizeof(char[200])
}
