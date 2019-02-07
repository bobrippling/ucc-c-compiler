// RUN: %check --only %s -Wno-attr-printf-voidptr

int printf(const char *, ...)
	__attribute((format(printf,1,2)));

int main(void)
{
	// should warn about this even without -Wattr-printf-voidptr
	printf("%p\n", 0); // CHECK: warning: %p expects a 'void *' argument, not 'int'
}
