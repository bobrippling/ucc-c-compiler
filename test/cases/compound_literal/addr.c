// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 2 ]

main()
{
	int *p = &(int){2};
	return *p;
}
