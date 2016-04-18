// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 5 ]

main()
{
	return 5;
}
