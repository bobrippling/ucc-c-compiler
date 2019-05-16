// RUN: %ucc -S -o %t %s
// RUN: grep cvttss2si %t >/dev/null
// RUN: grep addss %t >/dev/null
// RUN: ! grep addl %t >/dev/null

main()
{
	float a = 0;
	int i = 0;

	i = a++; // a loaded, converted to int, stored to i.
	         // a loaded, added to (float)1, stored
}
