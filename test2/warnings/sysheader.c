// RUN: %check %s -Weverything -xcpp-output -nostdinc -isystem sysheader

# 1 "sysheader/systemfile.h"

__attribute((bad_attribute)) extern int __abc; // CHECK: !/warn/

a; // CHECK: !/warn/
f(); // CHECK: !/warn/

# 11 "hello.c"

__attribute((bad_attribute)) extern int __xyz; // CHECK: /warn/

b; // CHECK: /warn/
g(); // CHECK: /warn/


main()
{
	f(a, __abc); // CHECK: !/warn/
	g(b, __xyz); // CHECK: !/warn/
}
