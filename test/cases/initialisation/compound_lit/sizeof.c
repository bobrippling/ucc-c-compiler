// RUN: %ucc %s -o %t
// RUN: %t; [ $? -eq 8 ]

main()
{
	return sizeof(int[]){1, 2};
}
