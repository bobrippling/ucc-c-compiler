// RUN: %ucc -S -o %t %s
// RUN: test `grep -c '0=\(.*\) 1=\1' %t` -eq 3

main()
{
	int i = 2, j = 7;

	__asm("0=%0 1=%1" : "=r"(i) : "0"(j));

	// j -> memory @ xyz
	// i <- memory @ xyz
	__asm("0=%0 1=%1" : "=m"(i) : "0"(j));

	__asm("0=%0 1=%1" : "=g"(i) : "0"(j));

	return i;
}
