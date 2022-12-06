#ifndef ALIGN
#define ALIGN(n) \
	_Alignas(n)
	//__attribute__((aligned(N)))
#endif

void use(void *);

char gbuf[4096] __attribute__((aligned(2048)));

int f() {
	// FIXME:
	// align local variable
	//
	// probestack?
	// - what do gcc/clang do?
	// [ -fstack-clash-protector ]

	ALIGN(2048) char buf[4096];

	use(buf);

	return 3;
}
