// RUN: %ucc -o %t %s
// RUN: %t

char bits2[_Generic((char)5, int: 4, char: 1)];

main()
{
	return sizeof(bits2) == 1 ? 0 : 1;
}
