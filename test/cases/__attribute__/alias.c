// RUN: %ucc -S -o %t %s -fpic -target x86_64-linux
// RUN: grep 'f = f_impl' %t
// RUN: grep 'static_f = f_impl' %t
// RUN: grep 'callq target_alias$' %t
//
// RUN: %check --only %s -DATTR=always_inline -fshow-inlined -fno-semantic-interposition

int f_impl()
{
	return 21;
}

extern __typeof(f_impl) f __attribute__((alias("f_impl")));

__attribute__((alias("f_impl")))
static int static_f();

// -------------------------------

#ifndef ATTR
#  define ATTR
#endif

__attribute((ATTR))
int target()
{
	return 3;
}

__attribute((alias("target")))
__attribute((ATTR))
static int target_alias(void);

int main()
{
	return target() // CHECK: note: function inlined
		+ target_alias(); // CHECK: note: function inlined
}
