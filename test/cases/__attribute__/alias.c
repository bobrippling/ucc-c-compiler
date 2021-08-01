// RUN: %ucc -S -o %t %s -fpic -target x86_64-linux
// RUN: grep 'f = f_impl' %t
// RUN: grep 'static_f = f_impl' %t
// RUN: grep 'callq target_alias$' %t
//
// RUN: grep '\.protected fn_protected$' %t
// RUN: grep '\.hidden fn_hidden$' %t
// RUN: grep '\.protected var_protected$' %t
// RUN: grep '\.hidden var_hidden$' %t
//
// RUN: grep '\.protected protected$' %t
// RUN: grep '\.hidden hidden$' %t
//
// RUN: %check --only %s -DATTR=always_inline -fshow-inlined -fno-semantic-interposition -target x86_64-linux
// ^ we must -target x86_64-linux so we can use visibility("protected")

int f_impl()
{
	return 21;
}

extern __typeof(f_impl) f __attribute__((alias("f_impl")));

__attribute__((alias("f_impl"), used))
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

void fn(){} // must alias a definition
__attribute((alias("fn"), visibility("hidden"))) void fn_hidden();
__attribute((alias("fn"), visibility("protected"))) void fn_protected();

int var;
__attribute((alias("var"), visibility("hidden"))) extern int var_hidden;
__attribute((alias("var"), visibility("protected"))) extern int var_protected;

__attribute((visibility("protected")))
void protected()
{
}
__attribute((visibility("hidden"), alias("protected")))
void hidden();
