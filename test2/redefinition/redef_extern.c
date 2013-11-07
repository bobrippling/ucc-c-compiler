// RUN: %check -e %s
main()
{
	extern int i; // CHECK: error: extern/non-extern definitions of "i"
	int i;
	i = 2;
	return i;
}
