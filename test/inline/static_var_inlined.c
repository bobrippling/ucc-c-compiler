// RUN: %ocheck 0 %s -fno-semantic-interposition
// RUN: %check %s -fno-semantic-interposition

__attribute((always_inline))
inline void *f(unsigned size)
{
	static char malloc_buf[16]; // CHECK: warning: mutable static variable in pure-inline function - may differ per file
	static void *ptr = malloc_buf; // CHECK: warning: mutable static variable in pure-inline function - may differ per file
	void *ret = ptr;

	ptr += size;

	return ret;
}

main()
{
	char *a = f(2);
	char *b = f(1);

	if(a + 2 != b)
		abort();

	return 0;
}
