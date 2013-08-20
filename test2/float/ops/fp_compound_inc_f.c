// RUN: %ucc -S -o %t %s
// RUN: grep > /dev/null cvtss2si %t
// RUN: grep > /dev/null addss %t
// RUN: grep > /dev/null addl %t; [ $? -ne 0 ]

main()
{
	float a = 0;
	int i = 0;

	i = a++; // a loaded, converted to int, stored to i.
	         // a loaded, added to (float)1, stored
}
