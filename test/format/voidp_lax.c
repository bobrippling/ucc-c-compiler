// RUN: %check --prefix=strict %s -Wattr-printf-voidptr
// RUN: %check --prefix=lax %s -Wno-attr-printf-voidptr

int printf(const char *, ...)
	__attribute((format(printf, 1, 2)));

main()
{
	void *vp = 0;
	int *ip = 0;

	// fine in all cases
	printf("%p\n", vp); // CHECK-strict: !/warn/
	                    // CHECK-lax: ^ !/warn/

	printf("%p\n", ip); // CHECK-strict: warning: format %p expects 'void *' argument (got int *)
	                    // CHECK-lax: ^ !/warn/
}
