// RUN: %ucc -S -o- %s -fpic -target x86_64-linux | grep 'callq quickf$' >/dev/null
// RUN: %check %s -DATTR=always_inline -fshow-inlined -fno-semantic-interposition

__attribute((ATTR))
int f()
{
	return 3;
}

#ifndef ATTR
#  define ATTR
#endif

__attribute((alias("f")))
__attribute((ATTR))
static int quickf(void);

int main()
{
	return f() // CHECK: note: function inlined
		+ quickf(); // CHECK: note: function inlined
}
