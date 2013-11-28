// RUN: %ucc -fshort-enums -fsyntax-only %s

main()
{
	enum via_uchar
	{
		A, B
	};
	SASSERT(sizeof(enum via_uchar) == sizeof(char));
	SASSERT(__builtin_is_signed(enum via_uchar) == 0);

	enum via_short
	{
		x = -1, y = 300 // > 255
	};
	SASSERT(sizeof(enum via_short) == sizeof(short));
	SASSERT(__builtin_is_signed(enum via_short));

	enum via_int
	{
		a = -1, b = 2_147_483_647
	};
	SASSERT(sizeof(enum via_int) == sizeof(int));
	SASSERT(__builtin_is_signed(enum via_int));

	enum via_long
	{
		a = -1, b = 2_147_483_648
	};
	SASSERT(sizeof(enum via_long) == sizeof(long));
	SASSERT(__builtin_is_signed(enum via_long));
}
