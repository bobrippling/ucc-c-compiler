// RUN: %check %s
// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f(int i)
{
	(void)(   0x3 ==((i & ~0xc) | 0x3)  );
	(void)(   0x3 == (i & ~0xc) | 0x3   ); // CHECK: warning: == has higher precedence than |

	(void)(   5 >= 2 & 3   ); // CHECK: warning: >= has higher precedence than &
}

main()
{
	// (1 && 0) || 0
	if(1 && 0 || 0)
		abort();

	if(0 && 1 || 0)
		abort();

	return 0;
}
