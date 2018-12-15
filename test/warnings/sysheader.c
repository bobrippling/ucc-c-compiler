// RUN: %check %s -Wimplicit-int -xcpp-output -nostdinc -isystem sysheader

# 1 "hello.c"

# 1 "sysheader/systemfile.h" 3

__attribute((bad_attribute)) extern int __abc; // CHECK: !/warn/

a; // CHECK: !/warn/
f(); // CHECK: !/warn/

# 13 "hello.c" 2

__attribute((bad_attribute)) extern int __xyz; // CHECK: /warn/

b; // CHECK: /warn/
g(); // CHECK: /warn/


main()
{
	f(a, __abc); // CHECK: !/warn/
	g(b, __xyz); // CHECK: !/warn/
}
