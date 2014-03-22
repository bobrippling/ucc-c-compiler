// RUN: %ucc -Xprint %s 2>/dev/null | grep 'typeof' | %output_check -w '/typeof\(A \*\) p1.*/' '/typeof\(int \*\) p2.*/' '/typeof\(A\) a1.*/' '/typeof\(A\) a2.*/'

typedef struct A { int i; } A;

f()
{
	__typeof(A *) p1;
	__typeof(int *) p2;
	__typeof(A) a1;
	__typeof(*p1) a2;
}

/*
__typeof(A *) p1
__typeof(int *) p2
__typeof(A) a1
__typeof(A) a2
*/
