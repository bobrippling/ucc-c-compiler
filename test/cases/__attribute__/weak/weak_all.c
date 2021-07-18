// RUN: %ucc -S -o %t %s -target x86_64-linux
// RUN: grep '\.weak f' %t
// RUN: grep '\.weak g' %t
// RUN: grep '\.weak x' %t
// RUN: grep '\.weak tdef' %t
// RUN: grep '\.weak extern_weak' %t
// RUN: grep '\.weak local' %t
// RUN: grep 'globl' %t | grep -v main; [ $? -ne 0 ]

__attribute((weak))
void f();

__attribute((weak))
void g()
{
}

__attribute((weak))
int x;

typedef int __attribute((weak)) weak_int;

weak_int tdef;

extern int extern_weak __attribute((weak));

main()
{
	extern int local __attribute((weak));
}
