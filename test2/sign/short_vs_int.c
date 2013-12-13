// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 1 ]
main()
{
	typedef unsigned short ty;
	ty a = 5, b = 20;

	// don't be fooled - this does a signed comparison due to promotion to int
	return (a - b) < 10;
}
