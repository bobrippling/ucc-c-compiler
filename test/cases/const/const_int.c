// RUN: %check -e %s

main()
{
	typedef const kint;
	kint x;

	x = 2; // CHECK: /can't modify const expression/
}
