// RUN: %ucc %s -o %t
// RUN: %t; [ $? -eq 20 ]
main()
{
	return __alignof(char *) + _Alignof(int) + __alignof__ 5 * 2; // 20
}
