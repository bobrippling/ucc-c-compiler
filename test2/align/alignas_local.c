// RUN: %ucc -DBOTH -c %s
// RUN: %ucc -DBOTH -S -o- %s | grep '\$1, *-8(%rsp)'
// RUN: %ucc -DBOTH -S -o- %s | grep '\$2, *-16(%rsp)'

f()
{
	// both x and y are 8-byte aligned
	_Alignas(8) int x = 1
#ifdef BOTH
		,
#else
		; int
#endif
		y = 2;

	g(x, y);
}
