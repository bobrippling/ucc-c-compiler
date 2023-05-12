long add(long a, long b) {
	long r;

	__asm("add %0, %1, %2, uxtx #1" : "=r"(r) : "r"(a), "r"(b));

	return r;
}

long add2(long a, long b) {
	int r;
	__asm("unknownop %0, %1, %2, uxtx #1" : "=r"(r) : "r"(a), "r"(b));
	//           ^~ should warn here because of `int r`
	//           but only on aarch64, since there we have to use %w0 to mean the 32-bit reg,
	//           whereas on x86_64, %0 can be either the 64-bit reg or the 32-bit reg, depending on the operand size

	return r;
}
