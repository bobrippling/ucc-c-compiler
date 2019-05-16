// RUN: %ucc -S -o %t %s
// RUN: grep incl %t >/dev/null
// RUN: ! grep addss %t >/dev/null

main()
{
	float a = 0;
	int i = 0;

	a = i++; // i -> float, stored to a. i += (int)1
}
