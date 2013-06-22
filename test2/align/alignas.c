// align of p is 8:
// RUN: %ucc -S -o- %s | grep -B1 '^p:' | sed -n 1p | grep ' 8'

char _Alignas(void *) p;

main()
{
	int _Alignas(8) i; // 8
	char _Alignas(int) c; // 4

	_Alignas(sizeof(int)) char c2; // +4

	_Alignas(void (*)()) pf; // 8

	_Alignas(8) _Alignas(4) _Alignas(16) int j; // 16

#define ALIGN(var, n) \
	_Static_assert(_Alignof(var) == n, #var " alignment not " #n)

	ALIGN(i, 8);
	ALIGN(c, 4);
	ALIGN(c2, 4);
	ALIGN(pf, 8);
	ALIGN(j, 16);
}
