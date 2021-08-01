// RUN: %check -e %s
main()
{
	extern int i;
	int i; // CHECK: error: extern/non-extern definitions of "i"
	i = 2;
	return i;
}
