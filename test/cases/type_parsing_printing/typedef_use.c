// RUN: %ucc -fsyntax-only %s

// see https://mobile.twitter.com/shafikyaghmour/status/1459558469472714758

typedef int T1;
void f1(void) {
	int x = (int)(enum{T1})1;
	x = (int)T1;
}


typedef long T2, U2;
enum {V2} (*f2(T2 T2, enum {U2} y, int x[T2+U2]))(T2 t);
T2 x[(U2)V2+1];


typedef signed int T3;
struct S {
	unsigned T3:3;
	const T3:3;
};


typedef int T4;
void f4(int(x), int(T4), int T4);
