// RUN: %check -e %s

main()
{
	int ar[2][f()] = { { 1, 2, 3 } }; // CHECK: error: cannot initialise variable length array
}
