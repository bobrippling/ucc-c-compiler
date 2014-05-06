// RUN: %ucc -S -o %t %s
// RUN: grep '\.weak _*f' %t
// RUN: grep '\.weak _*g' %t
// RUN: grep '\.weak _*x' %t
// RUN: grep '\.weak _*tdef' %t
// RUN: grep '\.weak _*extern_weak' %t
// RUN: grep '\.weak _*local' %t
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
