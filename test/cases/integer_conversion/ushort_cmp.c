// RUN: %ocheck 0 %s

typedef unsigned short uint16_t;

_Static_assert((uint16_t)-1 != (short)-1, "");

int main()
{
	uint16_t a = -1; // zext to int
	short b = -1; // sext to int

	if(a != b)
		return 0;

	return 1;
}
