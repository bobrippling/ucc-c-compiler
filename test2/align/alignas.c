// align of p is 8:
// RUN: %ucc -fsyntax-only %s

#define ALIGN(var, n) \
	_Static_assert(_Alignof(var) == n, #var " alignment not " #n)

char _Alignas(void *) p;

ALIGN(p, 8);

main()
{
	int _Alignas(8) i; // 8
	char _Alignas(int) c; // 4

	_Alignas(sizeof(int)) char c2; // +4

	_Alignas(void (*)()) pf; // 8

	_Alignas(8) _Alignas(4) _Alignas(16) int j; // 16

	ALIGN(i, 8);
	ALIGN(c, 4);
	ALIGN(c2, 4);
	ALIGN(pf, 8);
	ALIGN(j, 16);
}
