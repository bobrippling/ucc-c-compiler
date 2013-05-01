h(int[]); // int *
g(int()); // int(*)()


main()
{
	enum { A, B };

	f(A);
	g((int())A);

	int x[2];

	h(x);
}
