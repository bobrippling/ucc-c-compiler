// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 2 ]

_Bool x = 5;

main()
{
	_Bool b = 3;

	return b + (int)x;
}
