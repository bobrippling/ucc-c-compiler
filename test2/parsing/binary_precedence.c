// RUN: %check %s
f(int i)
{
	(void)(   0x3 ==((i & ~0xc) | 0x3)  );
	(void)(   0x3 == (i & ~0xc) | 0x3   ); // CHECK: warning: == has higher precedence than |

	(void)(   5 >= 2 & 3   ); // CHECK: warning: >= has higher precedence than &
}
