// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 5 ]

main()
{
	return _Generic('a', int: 5, char: 2);
}
