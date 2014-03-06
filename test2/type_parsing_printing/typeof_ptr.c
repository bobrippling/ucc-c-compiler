// RUN: %ucc -Xprint %s 2>/dev/null | grep 'typeof' | %output_check -w '/typeof\(A \*\) p1.*/' '/typeof\(int \*\) p2.*/' '/typeof\(A\) a1.*/' '/typeof\(A\) a2.*/'

typedef struct A { int i; } A;

f()
{
	typeof(A *) p1;
	typeof(int *) p2;
	typeof(A) a1;
	typeof(*p1) a2;
}

/*
typeof(A *) p1
typeof(int *) p2
typeof(A) a1
typeof(A) a2
*/
