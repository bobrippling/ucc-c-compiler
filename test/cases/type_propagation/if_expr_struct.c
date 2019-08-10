// RUN: %ucc -fsyntax-only %s

main()
{
	typedef struct
	{
		int i;
	} A;

	volatile A v;
	const A c;

	_Static_assert(
			0 == _Generic(0 ? c : v, A: 0, default: 1),
			"bad type prop");

}
