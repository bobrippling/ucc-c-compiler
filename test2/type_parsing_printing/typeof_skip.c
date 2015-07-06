// RUN: %ucc -emit=print %s > %t
// RUN: grep "ip 'typeof(int \*)'" %t
// RUN: grep "a1 'typeof(int \*) \*'" %t
// RUN: grep "a2 'typeof(int \*)\[2\]'" %t
// RUN: grep "a3 'typeof(int \*) ()'" %t
// RUN: grep "abc 'typeof(expr: identifier) (aka 'long \*') ()'" %t
// RUN: grep "xyz 'typeof(expr: identifier) (aka 'long \*') ()'" %t

long *x;

__typeof(int *) ip;

__typeof(int *) *a1;
__typeof(int *) a2[2];
__typeof(int *) a3();

auto abc() -> __typeof(x)
{
}

__typeof(x) xyz()
{
}
