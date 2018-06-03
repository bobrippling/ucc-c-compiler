//wchar_t r3[] = u"\xC0"; // 0xc0
//wchar_t r4[] = u"\u00D0"; // UTF-16 -> 0x00d0

char r2[] = "\u00D0"; // U+00D0, UTF-8 -> ???

_Static_assert(L'\uF00C' == 0xf00c, "");
_Static_assert(L'\U0010D00B' == 0x10d00b, "");

_Static_assert(L'\U0010F00B\uF00B' == L'\uF00B', "");

	//assert(r2[0] == 0xc0); ?
	//assert(r2[1] == 0xc3);
	//assert(r2[2] == 0x80);
	//assert(r2[3] == 0); oob

char test[] = "\u319f92a";

main()
{
	write(1, test, sizeof(test));
}
