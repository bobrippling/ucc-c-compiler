// RUN: %ucc -fsyntax-only %s
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	char x = *(char *)(long)"hi";

	printf("endian: %d\n", (*(unsigned short *)"\xff\x00" < 0x100));

	_Static_assert(
			__builtin_constant_p((char *)(long)"hi"),
			"word casts should be constant"); // but not necessarily dereference

	_Static_assert(
			!__builtin_constant_p(*(unsigned short *)"\xff\x00"),
			"endian dependent cast/deref result");
}
