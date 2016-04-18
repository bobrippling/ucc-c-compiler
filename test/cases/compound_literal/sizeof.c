// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 8 ]
main()
{
	return sizeof(int[]){2, 3};
}
