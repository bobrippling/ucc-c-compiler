// https://nickdesaulniers.github.io/blog/2020/04/06/off-by-two/

typedef _Bool bool;

#ifndef __UCC__
static
#endif
inline
__attribute__((__always_inline__)) // FIXME: __always_inline__ unrecognised
bool bar(bool loc) {
	__asm(".quad 42 + %c0 - .\n\t" : : "i" (loc)); // FIXME: test %c0
	return 1;
}

int foo(void) {
	return bar(1);
}
