// RUN: %ucc -o %t %s; [ $? -ne 0 ]

main()
{
	// two int cases
	return _Generic(2, int: 5, int: 2);
}
