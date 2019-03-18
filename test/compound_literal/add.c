// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 4 ]

main()
{
	return (int){1} + (int){3};
}
