// RUN: %ocheck 0 %s
// RUN: %ocheck 0 %s -fstack-protector-all

#define __u __attribute__((unused))

/*
 * this attempts to aggravate 16-byte stack alignment problems
 * in function call code generation
 */

// 6 x86 call registers: rdi, rsi, rdx, rcx, r8, r9
// 7 arguments - 7th passed via stack, meaning a stack subtraction of 8 bytes (machine word size)
//             - we then align the stack so the 7th argument gets shifted by 8 bytes relative to %rsp
//             - should preserve the 7th argument
int f(__u int a, __u int b, __u int c, __u int d, __u int e, __u int f, int seven)
{
	// 4 bytes, i.e int
	if(seven != 0x1234_5678)
		abort();
	return 0;
}

int main()
{
	f(0x98765432,
	  0x98765432,
	  0x98765432,
	  0x98765432,
	  0x98765432,
	  0x98765432,
	  0x1234_5678);

	return 0;
}
