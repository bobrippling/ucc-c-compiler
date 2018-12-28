// RUN: %check -e %s
main()
{
	int x[] = { sizeof x }; // CHECK: error: sizeof incomplete type int[]
}
