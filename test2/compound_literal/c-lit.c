// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 5 ]

f()
{
	int i = (int){2};
	return i;
}

main()
{
	return (int[]){1, 2, 3, 4}[2] + f();
}
