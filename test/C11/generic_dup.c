// RUN: %check -e %s

main()
{
	// two int cases
	return _Generic(2, int: 5, int: 2); // CHECK: error: duplicate type in _Generic: int
}
