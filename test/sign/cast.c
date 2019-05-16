// RUN: %ocheck 253 %s

#define E (unsigned)-3

_Static_assert((E & 0xff) == 253, "");

main()
{
	return E;
}
