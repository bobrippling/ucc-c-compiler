// RUN: %ucc %s -o %t

int remap(int c, int n2)
{
	// !n2 creates a V_FLAG/_Bool in the backend, despite being int in the frontend.
	// this tests that the backend still promotes it correctly for the addition

	return c + !n2;
}

main()
{
}
