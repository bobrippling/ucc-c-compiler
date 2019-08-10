// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 6 ]

add(int a, int b)
{
	return (int)&a[(char *)b];
}

main()
{
	return add(1, 5);
}
