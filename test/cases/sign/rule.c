// RUN: %asmcheck %s
main()
{
	// rule #1 - diff type
	// rule #2 - diff signedness
	// rule #3 - unsigned one < signed one
	// rule #4 - ? signed short can do all of unsigned int...
	signed   short i = 3;
	unsigned char  c = 2;

	return i + c;
}
