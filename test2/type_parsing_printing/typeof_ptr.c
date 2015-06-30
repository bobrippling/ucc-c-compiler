// RUN: %ucc -emit=print %s | grep 'typeof' > %t
// RUN: grep "p1 'typeof(A \*)'" %t
// RUN: grep "p2 'typeof(int \*)'" %t
// RUN: grep "a1 'typeof(A)'" %t
// RUN: grep "a2 'typeof(A)'" %t

typedef struct A { int i; } A;

f()
{
	__typeof(A *) p1;
	__typeof(int *) p2;
	__typeof(A) a1;
	__typeof(*p1) a2;
}
