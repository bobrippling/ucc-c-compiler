// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 2 ]

main()
{
	return _Generic('a', default: 2, char: 5);
}
