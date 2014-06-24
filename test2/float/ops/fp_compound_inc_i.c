// RUN: %ucc -S -o %t %s
// RUN: grep > /dev/null incl %t
// RUN: grep > /dev/null addss %t; [ $? -ne 0 ]

main()
{
	float a = 0;
	int i = 0;

	a = i++; // i -> float, stored to a. i += (int)1
}
