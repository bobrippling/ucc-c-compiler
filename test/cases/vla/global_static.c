// RUN: %check -e %s

int x[f()]; // CHECK: error: static-duration variable length array

main()
{
	static int y[f()]; // CHECK: error: static-duration variable length array
}
