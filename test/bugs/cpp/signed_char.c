#if '\0' - 1 > 0
char signed
#else
char unsigned
#endif

#if L'\0' - 1 > 0
wchar signed
#else
wchar unsigned
#endif

// may not work (as a unsigned-char test)
// see C11 clause 6.10.1 clause 4:
// 6.10.1 ¶4 defines preprocessor #if arithmetic to take place
// in intmax_t or uintmax_t, depending on the signedness of the integer
// operand types
//
// see also https://www.openwall.com/lists/musl/2020/05/13/19

#if '\xff' > 0
// and (less strict - see https://git.musl-libc.org/cgit/musl/commit/include/limits.h?id=cdbbcfb8f5d748f17694a5cc404af4b9381ff95f)
// #if '\0'-1 > 0

main() {
	char c = '\xff';
	unsigned char c_u = '\xff';
	signed char c_s = '\xff';

	printf("%d\n", c);
	printf("%d\n", c_u);
	printf("%d\n", c_s);
}
#endif
